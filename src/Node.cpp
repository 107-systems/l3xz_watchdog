/**
 * Copyright (c) 2023 LXRobotics GmbH.
 * Author: Alexander Entinger <alexander.entinger@lxrobotics.com>
 * Contributors: https://github.com/107-systems/l3xz_watchdog/graphs/contributors.
 */

/**************************************************************************************
 * INCLUDE
 **************************************************************************************/

#include <l3xz_watchdog/Node.h>

#include <sstream>
#include <fstream>

#include <nlohmann/json.hpp>

/**************************************************************************************
 * NAMESPACE
 **************************************************************************************/

namespace l3xz
{

/**************************************************************************************
 * CTOR/DTOR
 **************************************************************************************/

Node::Node()
: rclcpp::Node("l3xz_watchdog")
, _prev_watchdog_loop_timepoint{std::chrono::steady_clock::now()}
{
  declare_parameter("config_file", "watchdog-config.json");

  /* Load a list of all nodes to be monitored from
   * the JSON configuration file.
   */
  std::string const config_file_name = get_parameter("config_file").as_string();

  std::ifstream json_cfg_if(config_file_name.c_str());
  if (!json_cfg_if.good()) {
    RCLCPP_ERROR(get_logger(), "Could not open configuration file \"%s\".", config_file_name.c_str());
    rclcpp::shutdown();
    return;
  }
  nlohmann::json const watchdog_config = nlohmann::json::parse(json_cfg_if);
  json_cfg_if.close();

  for (auto &node_entry : watchdog_config["node_list"])
  {
    std::string const node = node_entry["node"];
    size_t const timeout_ms = node_entry["node_timeout_ms"];

    _heartbeat_monitor_map[node] = create_heartbeat_monitor(node, std::chrono::milliseconds(timeout_ms));
  }

  /* Setup periodically called function to check the
   * online status (determined via regular received
   * heartbeat messages) of all nodes under monitoring.
   */
  _watchdog_loop_timer = create_wall_timer
    (std::chrono::milliseconds(WATCHDOG_LOOP_RATE.count()), [this]() { this->watchdog_loop(); });

  RCLCPP_INFO(get_logger(), "Node started successfully.");
}

Node::~Node()
{
  RCLCPP_INFO(get_logger(), "Node shut down successfully.");
}

/**************************************************************************************
 * PRIVATE MEMBER FUNCTIONS
 **************************************************************************************/

void Node::watchdog_loop()
{
  auto const now = std::chrono::steady_clock::now();
  auto const watchdog_loop_rate = (now - _prev_watchdog_loop_timepoint);
  if (watchdog_loop_rate > (WATCHDOG_LOOP_RATE + std::chrono::milliseconds(10)))
    RCLCPP_WARN_THROTTLE(get_logger(),
                         *get_clock(),
                         1000,
                         "watchdog_loop should be called every %ld ms, but is %ld ms instead",
                         WATCHDOG_LOOP_RATE.count(),
                         std::chrono::duration_cast<std::chrono::milliseconds>(watchdog_loop_rate).count());
  _prev_watchdog_loop_timepoint = now;

  /* Iterate over all registered heartbeat monitors and
   * check if a heartbeat timeout has occurred on any
   * of those.
   */
  for (auto [name, monitor] : _heartbeat_monitor_map)
  {
    if (monitor->isTimeout()) {
      RCLCPP_ERROR(get_logger(), "Node \"%s\" heart beat signal has timed out.", name.c_str());
    }
  }
}

HeartbeatMonitor::SharedPtr Node::create_heartbeat_monitor(std::string const & node, std::chrono::milliseconds const node_timeout)
{
  std::stringstream heartbeat_topic;
  heartbeat_topic << "/l3xz/" << node << "/heartbeat";

  return std::make_shared<HeartbeatMonitor>(heartbeat_topic.str(),
                                            node_timeout,
                                            *this);
}

/**************************************************************************************
 * NAMESPACE
 **************************************************************************************/

} /* l3xz */
