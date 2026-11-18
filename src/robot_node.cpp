#include <memory>
#include <string>
#include <std_msgs/msg/string.hpp>
#include <rclcpp/rclcpp.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <chrono>
#include <nlohmann/json.hpp>

class RobotNode : public rclcpp::Node {
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr motor_command_pub_;

    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr esp32_status_sub_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr motor_status_sub_;
    boost::asio::io_context context_;
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_{context_};

    boost::beast::flat_buffer receive_buffer_;
    bool sending_ = false;
    rclcpp::TimerBase::SharedPtr network_timer_;
    rclcpp::TimerBase::SharedPtr status_timer_;

    void send_motor_command(const std::string &command) {
        std_msgs::msg::String message;
        message.data = command;
        motor_command_pub_->publish(message);
    }

    void read_server() {
        ws_.async_read(
            receive_buffer_,
            [this](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    RCLCPP_WARN(
                        get_logger(), "server receive failed: %s",
                        ec.message().c_str());
                    return;
                }
                const auto message = boost::beast::buffers_to_string(receive_buffer_.data());
                const auto data = nlohmann::json::parse(message, nullptr, false);

                if (data.is_object() &&
                    data.contains("type") &&
                    data["type"] == "motor_command" &&
                    data.contains("command") &&
                    data["command"].is_string()) {
                    const auto command = data["command"].get<std::string>();

                    if (command == "forward" || command == "stop" ||
                        command == "backward" || command == "left" ||
                        command == "right") {
                        send_motor_command(command);
                    }
                }
                RCLCPP_INFO(
                    get_logger(), "server message: %s", message.c_str());
                receive_buffer_.consume(receive_buffer_.size());
                read_server();
            });
    }


    void connect_server() {
        boost::asio::ip::tcp::resolver resolver(context_);
        auto addresses = resolver.resolve("43.201.10.151", "80");

        boost::asio::connect(ws_.next_layer(), addresses);
        ws_.handshake("43.201.10.151:80", "/ws/robot");
        ws_.text(true);
    }

    void send_status(const std::string) {
        // 이미 전송 중이면 이번 값은 버린다. 따로 쌓아두지 않는다.
        if (sending_) {
            return;
        }

        if (!ws_.is_open()) {
            RCLCPP_WARN(get_logger(), "server is not connected");
            return;
        }

        // nlohmann::json status = nlohmann::json::parse(esp32_status);

        nlohmann::json status = {{"jetson_status", 1}};
        auto message = std::make_shared<std::string>(status.dump());
        sending_ = true;

        // 전송을 시작하고 바로 돌아온다.
        ws_.async_write(
            boost::asio::buffer(*message),
            [this, message](boost::system::error_code ec, std::size_t) {
                // 전송이 끝났으므로 다음 데이터를 보낼 수 있다.
                sending_ = false;

                if (ec) {
                    RCLCPP_WARN(
                        get_logger(), "server send failed: %s",
                        ec.message().c_str());
                }
            });
    }

public:
    RobotNode() : Node("robot_node") {
        motor_command_pub_ = create_publisher<std_msgs::msg::String>(
            "/robot/motor_command", 10
        );

        connect_server();
        read_server();

        network_timer_ = create_wall_timer(
            std::chrono::milliseconds(10),
            [this]() {
                context_.restart();
                context_.poll();
            });
        status_timer_ = create_wall_timer(
            std::chrono::seconds(1),
            [this]() {
                send_status("");
            });
        motor_status_sub_ = create_subscription<std_msgs::msg::String>(
            "/motor/status",
            10,
            [this](std_msgs::msg::String::ConstSharedPtr msg) {
                RCLCPP_INFO(
                    get_logger(), "motor status: %s", msg->data.c_str()
                );
            }
        );
        // esp32 구독자
        // esp32_status_sub_ = create_subscription<std_msgs::msg::String>(
        //     "/esp32/status",
        //     10,
        //     [this](std_msgs::msg::String::ConstSharedPtr msg) {
        //         RCLCPP_INFO(get_logger(), "esp32_status: %s", msg->data.c_str());
        //         send_status(msg->data);
        //     });
    }
};

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<RobotNode>());
    rclcpp::shutdown();
    return 0;
}
