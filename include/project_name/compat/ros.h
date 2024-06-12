#ifndef COMPAT_ROS_H
#define COMPAT_ROS_H

#if PROJECT_NAME_ROS_VERSION == 1
#include <ros/callback_queue.h>
#include <ros/ros.h>

// Commonly used built-in interfaces
#include <std_msgs/String.h>

// User-defined message interfaces
// #include <project_name/MyMessage.h>

// User-defined service interfaces
// #include <project_name/MyService.h>

namespace std_msgs_compat = std_msgs;

#elif PROJECT_NAME_ROS_VERSION == 2
#include <rclcpp/rclcpp.hpp>

// Commonly used built-in interfaces
#include <std_msgs/msg/string.hpp>

// User-defined message interfaces
// #include <project_name/MyMessage.hpp>

// User-defined service interfaces
// #include <project_name/MyService.hpp>

namespace std_msgs_compat = std_msgs::msg;

#endif

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace project_name::compat {

#if PROJECT_NAME_ROS_VERSION == 1

  using namespace ::project_name;

  template<typename T>
  using RawRequestType = typename T::Request;

  template<typename T>
  using RawResponseType = typename T::Response;

  template<typename T>
  using RequestType = typename T::Request;

  template<typename T>
  using ResponseType = typename T::Response;

  template<typename T, typename Request_ = typename T::Request>
  inline auto makeRequest() { return Request_(); }

  template<typename T, typename Response_ = typename T::Response>
  inline auto makeResponse() { return Response_(); }

  // todo: RequestType, ResponseType

#elif PROJECT_NAME_ROS_VERSION == 2
  using namespace ::project_name::msg;
  using namespace ::project_name::srv;

  template<typename T>
  using RawRequestType = typename T::Request;

  template<typename T>
  using RawResponseType = typename T::Response;

  template<typename T>
  using RequestType = std::shared_ptr<typename T::Request>;

  template<typename T>
  using ResponseType = std::shared_ptr<typename T::Response>;

  template<typename T, typename Request_ = typename T::Request>
  inline auto makeRequest() { return std::make_shared<Request_>(); }

  template<typename T, typename Response_ = typename T::Response>
  inline auto makeResponse() { return std::make_shared<Response_>(); }

// template <typename T, typename Result_ = typename T::>
#endif

  namespace project_name_ros {

#if PROJECT_NAME_ROS_VERSION == 1
    template<typename T>
    using ServiceWrapper = T;

    template<typename T>
    using MessageWrapper = typename T::ConstPtr;

    using Rate = ros::Rate;
    using RosTime = ros::Time;

    template<typename T>
    T* getServicePointer(T& service) { return &service; }

#elif PROJECT_NAME_ROS_VERSION == 2
    template<typename T>
    using ServiceWrapper = typename T::SharedPtr; // std::shared_ptr<T>;

    template<typename T>
    using MessageWrapper = typename T::ConstSharedPtr;

    using Rate = rclcpp::Rate;
    using RosTime = rclcpp::Time;

    using namespace ::project_name::msg;
    using namespace ::project_name::srv;

    template<typename T>
    T& getServicePointer(T& service) { return service; }
#endif

    template<typename T>
    class Publisher;

    template<typename T>
    class Subscriber;

    template<typename T>
    class Service;

    template<typename T>
    class Client;

    class Time : public RosTime
    {
    public:
      Time(uint32_t sec, uint32_t nsec) : RosTime((int32_t)sec, (int32_t)nsec) {}
      explicit Time(int64_t t) : RosTime(t) {}
      Time(const RosTime& time) : RosTime(time) {} // do not put it as explicit

      uint32_t seconds() const
      {
#if PROJECT_NAME_ROS_VERSION == 1
        return sec;
#elif PROJECT_NAME_ROS_VERSION == 2
        return (uint32_t)RosTime::seconds();
#endif
      }

      uint32_t nanoseconds() const
      {
#if PROJECT_NAME_ROS_VERSION == 1
        return nsec;
#elif PROJECT_NAME_ROS_VERSION == 2
        return RosTime::nanoseconds();
#endif
      }
    };

    class Node
    {
    public:
      template<typename T>
      friend class Publisher;

      template<typename T>
      friend class Subscriber;

      template<typename T>
      friend class Service;

      template<typename T>
      friend class Client;

      Node(Node& other) = delete;
      Node(Node&& other) = delete;
      ~Node() = default;

      static Node& get();
      static bool ok();

      static void init(int argc, char** argv, const std::string& node_name);
      static void shutdown();

      void spin();

      Time currentTime();

    private:
      explicit Node(const std::string& node_name);

      const std::string name_;

#if PROJECT_NAME_ROS_VERSION == 1
      ros::NodeHandle handle_;
      ros::CallbackQueue callback_queue_;
#elif PROJECT_NAME_ROS_VERSION == 2
      rclcpp::Node::SharedPtr handle_;
      std::thread ros_thread_;
#endif

      bool running_;
    };

    template<typename T>
    class Publisher
    {
    public:
      Publisher(const std::string& topic_name, std::size_t queue_size)
      {
        auto& node = Node::get();

#if PROJECT_NAME_ROS_VERSION == 1
        handle_ = node.handle_.advertise<T>(topic_name, queue_size);
#elif PROJECT_NAME_ROS_VERSION == 2
        (void)queue_size;
        handle_ = node.handle_->create_publisher<T>(topic_name, 10);
#endif
      }

      void publish(const T& message)
      {
#if PROJECT_NAME_ROS_VERSION == 1
        handle_.publish(message);
#elif PROJECT_NAME_ROS_VERSION == 2
        handle_->publish(message);
#endif
      }

      size_t getNumSubscribers()
      {
#if PROJECT_NAME_ROS_VERSION == 1
        return handle_.getNumSubscribers();
#elif PROJECT_NAME_ROS_VERSION == 2
        return handle_->get_subscription_count();
#endif
      }

    private:
#if PROJECT_NAME_ROS_VERSION == 1
      ros::Publisher handle_;
#elif PROJECT_NAME_ROS_VERSION == 2
      typename rclcpp::Publisher<T>::SharedPtr handle_;
#endif
    };

    template<typename T>
    class Subscriber
    {
    public:
      template<typename Ta, typename Tb>
      Subscriber(const std::string& topic_name, std::size_t queue_size, Ta&& callback, Tb&& ptr)
      {
        auto& node = Node::get();

#if PROJECT_NAME_ROS_VERSION == 1
        handle_ = node.handle_.subscribe(topic_name, queue_size, callback, ptr);
#elif PROJECT_NAME_ROS_VERSION == 2
        (void)queue_size;
        handle_ = node.handle_->create_subscription<T>(topic_name, 10, std::bind(std::forward<Ta>(callback), ptr, std::placeholders::_1));
#endif
      }

    private:
#if PROJECT_NAME_ROS_VERSION == 1
      ros::Subscriber handle_;
#elif PROJECT_NAME_ROS_VERSION == 2
      typename rclcpp::Subscription<T>::SharedPtr handle_;
#endif
    };

    template<typename T>
    class Service
    {
    public:
      template<typename Ta>
      Service(const std::string& service_name, Ta&& callback)
      {
        auto& node = Node::get();

#if PROJECT_NAME_ROS_VERSION == 1
        handle_ = node.handle_.advertiseService(service_name, callback);
#elif PROJECT_NAME_ROS_VERSION == 2
        handle_ = node.handle_->create_service<T>(service_name, [&](compat::project_name_ros::ServiceWrapper<typename T::Request> req, compat::project_name_ros::ServiceWrapper<typename T::Response> res) { callback(req, res); });
        // handle_ = node.handle_->create_service<T>(service_name, callback);
#endif
      }

      template<typename Ta, typename Tb>
      Service(const std::string& service_name, Ta&& callback, Tb&& ptr)
      {
        auto& node = Node::get();

#if PROJECT_NAME_ROS_VERSION == 1
        handle_ = node.handle_.advertiseService(service_name, callback, ptr);
#elif PROJECT_NAME_ROS_VERSION == 2
        handle_ = node.handle_->create_service<T>(service_name, [ptr, callback](compat::project_name_ros::ServiceWrapper<typename T::Request> req, compat::project_name_ros::ServiceWrapper<typename T::Response> res) { (ptr->*callback)(req, res); });
        // handle_ = node.handle_->create_service<T>(service_name, std::bind(std::forward<Ta>(callback), ptr, std::placeholders::_1, std::placeholders::_2));
#endif
      }

    private:
#if PROJECT_NAME_ROS_VERSION == 1
      ros::ServiceServer handle_;
#elif PROJECT_NAME_ROS_VERSION == 2
      typename rclcpp::Service<T>::SharedPtr handle_;
#endif
    };

    template<typename T>
    class Client
    {
    public:
      enum class Status_e
      {
        ros_status_successful,
        ros_status_successful_with_retry,
        ros_status_failure
      };

      explicit Client(const std::string& service_name) : name_(service_name)
      {
        auto& node = Node::get();

#if PROJECT_NAME_ROS_VERSION == 1
        handle_ = node.handle_.serviceClient<T>(service_name, true);
#elif PROJECT_NAME_ROS_VERSION == 2
        handle_ = node.handle_->create_client<T>(service_name);
#endif
      }

      Status_e call(const project_name::compat::RequestType<T>& req, project_name::compat::ResponseType<T>& res)
      {
        using namespace std::chrono_literals;
        auto status = Status_e::ros_status_failure;

#if PROJECT_NAME_ROS_VERSION == 1
        T srv;
        srv.request = req;
        if(!handle_.call(srv))
        {
          auto& node = Node::get();
          handle_ = node.handle_.serviceClient<T>(name_, true);
          if(handle_.call(srv))
          {
            status = Status_e::ros_status_successful_with_retry;
            res = srv.response;
          }
        }
        else
        {
          status = Status_e::ros_status_successful;
          res = srv.response;
        }

#elif PROJECT_NAME_ROS_VERSION == 2
        if(!handle_->wait_for_service(5s))
        {
          return status;
        }

        auto future = handle_->async_send_request(req);

        if(future.wait_for(5s) == std::future_status::ready)
        {
          status = Status_e::ros_status_successful;
          res = future.get();
        }
#endif
        return status;
      }

      bool wait(double timeout)
      {
#if PROJECT_NAME_ROS_VERSION == 1
        return handle_.waitForExistence(ros::Duration(timeout));
#elif PROJECT_NAME_ROS_VERSION == 2
        return handle_->wait_for_service(std::chrono::duration<double>(timeout));
#endif
      }

    private:
      std::string name_;
#if PROJECT_NAME_ROS_VERSION == 1
      ros::ServiceClient handle_;
#elif PROJECT_NAME_ROS_VERSION == 2
      typename rclcpp::Client<T>::SharedPtr handle_;
#endif
    };

  } // namespace project_name_ros

} // namespace project_name::compat

#endif // COMPAT_ROS_H