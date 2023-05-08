/**
 * Copyright (c) 2023 LXRobotics GmbH.
 * Author: Alexander Entinger <alexander.entinger@lxrobotics.com>
 * Contributors: https://github.com/107-systems/l3xz_watchdog/graphs/contributors.
 */

#pragma once

/**************************************************************************************
 * INCLUDE
 **************************************************************************************/

#include <map>
#include <string>
#include <chrono>

#include <rclcpp/rclcpp.hpp>

#include "HeartbeatMonitor.h"

/**************************************************************************************
 * NAMESPACE
 **************************************************************************************/

namespace l3xz
{

/**************************************************************************************
 * CLASS DECLARATION
 **************************************************************************************/

class Node : public rclcpp::Node
{
public:
  Node();
  ~Node();

private:
  std::map<std::string, HeartbeatMonitor::SharedPtr> _heartbeat_monitor_map;

  std::chrono::steady_clock::time_point _prev_watchdog_loop_timepoint;
  static std::chrono::milliseconds constexpr WATCHDOG_LOOP_RATE{100};
  rclcpp::TimerBase::SharedPtr _watchdog_loop_timer;
  void watchdog_loop();

  HeartbeatMonitor::SharedPtr create_heartbeat_monitor(std::string const & node, std::chrono::milliseconds const node_timeout);
};

/**************************************************************************************
 * NAMESPACE
 **************************************************************************************/

} /* l3xz */
