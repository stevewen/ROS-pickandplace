/*********************************************************************
 * Software License Agreement (BSD License)
 *********************************************************************
 Author:      Alejandro Acevedo
 Modified by: Daniel Ordonez - daniels.ordonez@gmail.com    10/May/2018
*/

#include <moveit_msgs/DisplayRobotState.h>
#include <moveit_msgs/DisplayTrajectory.h>
#include <moveit_visual_tools/moveit_visual_tools.h>
#include <math.h>

#include <moveit_visual_tools/moveit_visual_tools.h>
#include <moveit_msgs/AttachedCollisionObject.h>
#include <moveit_msgs/CollisionObject.h>
#include <moveit/move_group_interface/move_group_interface.h>
//For add object
#include "geometric_shapes/shapes.h"
#include "geometric_shapes/mesh_operations.h"
#include "geometric_shapes/shape_operations.h"
#include <moveit/planning_scene_interface/planning_scene_interface.h>
#include <geometric_shapes/shape_operations.h>
// Logging capabilities
#include <ros/console.h>

using namespace Eigen;

int main(int argc, char **argv){

// Initial CONFIGURATION
// ************************************************************************************************
  ros::init(argc, argv, "invite_pick_and_place");
  ros::NodeHandle node_handle;
  ros::AsyncSpinner spinner(1);
  spinner.start();

  // Change console log level to DEBUG. (Optional)
  if( ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME, ros::console::levels::Debug) ) {
    ros::console::notifyLoggerLevelsChanged();
  }

  // Read Ros parameter indicating whther we are operating a simulation or the real robot
  // Variable used for detecting whether a motion plan was succesful.
  bool success;

  // Set up move group objects
  moveit::planning_interface::MoveGroupInterface right_gripper_move_group("right_gripper");
  right_gripper_move_group.allowReplanning(true);     // Allow move group to re plan motions when scene changes are detected
  moveit::planning_interface::MoveGroupInterface arm_right_move_group("arm_right");
  arm_right_move_group.allowReplanning(true);     // Allow move group to re plan motions when scene changes are detected
  moveit::planning_interface::MoveGroupInterface arm_left_move_group("arm_left");
  arm_left_move_group.allowReplanning(true);     // Allow move group to re plan motions when scene changes are detected
  moveit::planning_interface::MoveGroupInterface csda10f_move_group("csda10f");
  csda10f_move_group.allowReplanning(true);      // Allow move group to re plan motions when scene changes are detected
  // Raw pointers are frequently used to refer to the planning group for improved performance.
  const robot_state::JointModelGroup *csda10f_joint_model_group = csda10f_move_group.getCurrentState()->getJointModelGroup("csda10f");
  const robot_state::JointModelGroup *arm_left_joint_model_group = arm_left_move_group.getCurrentState()->getJointModelGroup("arm_left");
  const robot_state::JointModelGroup *arm_right_joint_model_group = arm_right_move_group.getCurrentState()->getJointModelGroup("arm_right");

  // Holder for motion plans.
  moveit::planning_interface::MoveGroupInterface::Plan my_plan;
  // RobotState instance that will hold move group robot states instances.
  robot_state::RobotState start_state(*csda10f_move_group.getCurrentState());

  // We will use the :planning_scene_interface:`PlanningSceneInterface`
  // class to add and remove collision objects in our "virtual world" scene
  moveit::planning_interface::PlanningSceneInterface planning_scene_interface;
  // Define visualization parameters
  namespace rvt = rviz_visual_tools;
  moveit_visual_tools::MoveItVisualTools visual_tools("base_link");
  visual_tools.deleteAllMarkers();
  visual_tools.loadRemoteControl();

  // Set up tutorial text position
  Eigen::Affine3d text_pose = Eigen::Affine3d::Identity();
  text_pose.translation().z() = 2.00; // above head of CSDA10F

  visual_tools.publishText(text_pose, "Robot Pick and Place Demo \n Press next to start", rvt::WHITE, rvt::XXLARGE);
  visual_tools.trigger();
  visual_tools.prompt("Press the 'next' button on the 'RvizVisualToolsGui' pannel");

// *****************************************************************************************
// STEP 1: Go to home position.

  // Use the previously defined home position for ease of motion, this position was defined on the moveit_config package
  csda10f_move_group.setStartState(*csda10f_move_group.getCurrentState());
  csda10f_move_group.setNamedTarget("home_arms_folded");
  success = (csda10f_move_group.plan(my_plan) == moveit::planning_interface::MoveItErrorCode::SUCCESS);
  visual_tools.deleteAllMarkers();
  visual_tools.publishText(text_pose, "Go Back Home Position", rvt::WHITE, rvt::XXLARGE);
  visual_tools.publishTrajectoryLine(my_plan.trajectory_, csda10f_joint_model_group);
  // visual_tools.trigger();
  // visual_tools.prompt("Press the 'next' button on the 'RvizVisualToolsGui' pannel");

  // Perform motion on real robot
  csda10f_move_group.execute(my_plan);
  ros::Duration(1.5).sleep();

// *****************************************************************************************
// STEP 2: Add manipulation object.
  visual_tools.publishText(text_pose, "Loading mesh model...", rvt::WHITE, rvt::XXLARGE);

  //Vector to scale
  Vector3d vectorScale(0.001, 0.001, 0.001);
  // Define a collision object ROS message.
  moveit_msgs::CollisionObject object;
  // The id of the object is used to identify it.
  object.id = "glass";

  //Path where is located le model
  shapes::Mesh* m = shapes::createMeshFromResource("package://robot_planning/meshes/glass.stl", vectorScale);
  // Define and load the mesh
  shape_msgs::Mesh mesh;
  shapes::ShapeMsg mesh_msg;
  shapes::constructMsgFromShape(m, mesh_msg);
  mesh = boost::get<shape_msgs::Mesh>(mesh_msg);
  ROS_INFO("Object mesh to manipulate...loaded");

  object.meshes.resize(1);
  object.mesh_poses.resize(1);

  //Define a pose for the object (specified relative to frame_id)
  geometry_msgs::Pose mesh_pose;
  mesh_pose.position.x = 0.6;
  mesh_pose.position.y = 0.0;
  mesh_pose.position.z = 0.82;     
  mesh_pose.orientation.x= 0.0;
  mesh_pose.orientation.y= 0.0;
  mesh_pose.orientation.z= 0.0;
  mesh_pose.orientation.w= 1.0;

  //The object is added like un colission object
  object.meshes.push_back(mesh);
  object.mesh_poses.push_back(mesh_pose);
  object.operation = object.ADD;      // operations are be object.REMOVE, object.APPEND, object.MOVE

  // Create vector of collision object messages for the planning_scene_interface
  std::vector<moveit_msgs::CollisionObject> objects;
  objects.push_back(object);

  // Add the collision objects into the world
  planning_scene_interface.addCollisionObjects(objects);
  ros::Duration(1.5).sleep();
  visual_tools.deleteAllMarkers();
  visual_tools.publishText(text_pose, "Object added into the world \n Press next to pick", rvt::PINK, rvt::XXLARGE);
  visual_tools.trigger();
  visual_tools.prompt("Press the 'next' button on the 'RvizVisualToolsGui' pannel");


  // ***********************************************************************************************
  // STEP 3: APPROACH CUP

  // Set up target pose for left arm.
  geometry_msgs::Pose approach_pose = mesh_pose;    // Use target object pose (x,y,z) coordinates as reference.
  tf::Quaternion orientation; 
  orientation.setRPY(M_PI/2 , -M_PI/2 , M_PI/4);  
  approach_pose.orientation.x = orientation.x();
  approach_pose.orientation.y = orientation.y();
  approach_pose.orientation.z = orientation.z();
  approach_pose.orientation.w = orientation.w();
  approach_pose.position.z += 0.04;                 // Move 8 cm above the target object object.
  approach_pose.position.x += 0.03;
  approach_pose.position.y += 0.03;

  arm_left_move_group.setStartState(*arm_left_move_group.getCurrentState());
  arm_left_move_group.setPoseTarget(approach_pose);
  // Compute motion plan
  success = (arm_left_move_group.plan(my_plan) == moveit::planning_interface::MoveItErrorCode::SUCCESS);
  ROS_INFO("Pick Tutorial: picking movements %s", success ? "SUCCESS" : "FAILED");

  // Visualizing plan
  visual_tools.deleteAllMarkers();
  visual_tools.publishAxisLabeled(approach_pose, "Picking Position");
  visual_tools.publishText(text_pose, "Planning for object grasping \n Press next to perform motion on the real robot", rvt::WHITE, rvt::XLARGE);
  visual_tools.publishTrajectoryLine(my_plan.trajectory_, arm_left_joint_model_group);
  // visual_tools.trigger();
  // visual_tools.prompt("Press the 'next' button on the 'RvizVisualToolsGui' pannel");

  // Once user allow it, exceute the planned trajectory
  ROS_INFO("Performing motion on real robot...");
  arm_left_move_group.execute(my_plan);
  ros::Duration(1.5).sleep();


//************************************************************************************************
// STEP 4: ATTACH THE OBJECT TO THE GRIPPER

  moveit::planning_interface::MoveGroupInterface left_gripper_mg("left_gripper");
  left_gripper_mg.attachObject(object.id);

  // Show text in Rviz of status
  visual_tools.publishText(text_pose, "Object attached to the gripper \n Press next to place", rvt::PINK, rvt::XXLARGE);
  visual_tools.trigger();
  visual_tools.prompt("Press the 'next' button on the 'RvizVisualToolsGui' pannel");

  // Sleep to allow MoveGroup to recieve and process the attached collision object message
  // ros::Duration(1.0).sleep();

//************************************************************************************************
// STEP 5: PLACE THE OBJECT

  visual_tools.publishText(text_pose, "Placing Object inside the drum...\n (this might take a few seconds)", rvt::WHITE, rvt::XXLARGE);

  // Set drop position *******************************
  geometry_msgs::Pose drop_pose = approach_pose;
  tf::Quaternion orient;
  orient.setRPY(M_PI/2 , -M_PI/2 , M_PI);   
  approach_pose.orientation.x = orient.x();
  approach_pose.orientation.y = orient.y();
  approach_pose.orientation.z = orient.z();
  approach_pose.orientation.w = orient.w();
  drop_pose.position.x = 1.0;    // 1.0            //[meters]
  drop_pose.position.y = 0.3;    // 0.3            //[meters]
  drop_pose.position.z = 0.9;    // 0.9            //[meters]
  csda10f_move_group.setStartState(*csda10f_move_group.getCurrentState());
  csda10f_move_group.setPoseTarget(drop_pose, "arm_left_link_tcp");

  // This movement will involve the manipulation of the 15 joints at the same time, and will be a long sweep, that is why we give some seconds to the solver
  // to find a solution...
  csda10f_move_group.setPlanningTime(20.0);
  csda10f_move_group.setMaxAccelerationScalingFactor(0.5);
  csda10f_move_group.setMaxVelocityScalingFactor(0.2);
  csda10f_move_group.setNumPlanningAttempts(3);
  //Even do it will take time replanning improves results... we are not in a hurry.

  // Plan the motion with constraints.
  success = (csda10f_move_group.plan(my_plan) == moveit::planning_interface::MoveItErrorCode::SUCCESS);
  // If no solution is found for dropping the object into the drum, roll back to home position and try again.
  if(!success){
    ROS_DEBUG("Planning from table to drum was not possible, going back to home position for replanning");
    // Use the previously defined home position for ease of motion, this position was defined on the moveit_config package
    csda10f_move_group.setStartState(*csda10f_move_group.getCurrentState());
    csda10f_move_group.setNamedTarget("home_arms_folded");
    success = (csda10f_move_group.plan(my_plan) == moveit::planning_interface::MoveItErrorCode::SUCCESS);
    visual_tools.deleteAllMarkers();
    visual_tools.publishText(text_pose, "Planning to drop position not feasible \n Comming back to home position for replanning...", rvt::WHITE, rvt::XXLARGE);
    visual_tools.publishTrajectoryLine(my_plan.trajectory_, csda10f_joint_model_group);
    visual_tools.trigger();
    visual_tools.prompt("Press the 'next' button on the 'RvizVisualToolsGui' pannel");
    // Perform motion on real robot
    csda10f_move_group.execute(my_plan);
    ros::Duration(1.5).sleep();
    // Replan to target position
    ROS_DEBUG("Replanning to target position...");
    csda10f_move_group.setStartState(*csda10f_move_group.getCurrentState());
    csda10f_move_group.setPoseTarget(drop_pose, "arm_left_link_tcp");
    success = (csda10f_move_group.plan(my_plan) == moveit::planning_interface::MoveItErrorCode::SUCCESS);
  }
  if(!success){
    ROS_ERROR("No planning found to target position ... :c");
    return 1;
  }
  // Visualize the plan in Rviz
  visual_tools.deleteAllMarkers();
  visual_tools.publishAxisLabeled(approach_pose, "start");
  visual_tools.publishAxisLabeled(drop_pose, "goal");
  visual_tools.publishText(text_pose, "Perform motion of the cup mantaining orientation to avoid liquid spilling \n Press next to perform motion on real robot", rvt::WHITE, rvt::XLARGE);
  visual_tools.publishTrajectoryLine(my_plan.trajectory_, csda10f_joint_model_group);
  // visual_tools.trigger();
  // visual_tools.prompt("Press the 'next' button on the 'RvizVisualToolsGui' pannel");

  // Once user allow it, exceute the planned trajectory
  ROS_INFO("Performing motion on real robot...");
  csda10f_move_group.execute(my_plan);
  ros::Duration(1.5).sleep();


  // Now, let's detach the collision object from the robot.
  ROS_INFO("Detach the object from the robot");
  left_gripper_mg.detachObject(object.id);

  // When done with the path constraint be sure to clear it.
  csda10f_move_group.clearPathConstraints();

  //************************************************************************************************
  // STEP 5.1: The right gripper grasp.

  // Use the previously defined home position for ease of motion, this position was defined on the moveit_config package
  right_gripper_move_group.setStartState(*right_gripper_move_group.getCurrentState());
  right_gripper_move_group.setNamedTarget("grasp_right_gripper");
  success = (right_gripper_move_group.plan(my_plan) == moveit::planning_interface::MoveItErrorCode::SUCCESS);
  visual_tools.publishText(text_pose, "The right gripper grasp", rvt::WHITE, rvt::XXLARGE);
  // visual_tools.trigger();
  // visual_tools.prompt("Press the 'next' button on the 'RvizVisualToolsGui' pannel");

  // Perform motion on real robot
  right_gripper_move_group.execute(my_plan);
  ros::Duration(2.5).sleep();

  // *****************************************************************************************
  // STEP 6: PLAN TO PRE TEACH HOME POSITION

  // Use the previously defined home position for ease of motion, this position was defined on the moveit_config package
  csda10f_move_group.setStartState(*csda10f_move_group.getCurrentState());
  csda10f_move_group.setNamedTarget("home_arms_open");
  success = (csda10f_move_group.plan(my_plan) == moveit::planning_interface::MoveItErrorCode::SUCCESS);
  visual_tools.deleteAllMarkers();
  visual_tools.publishText(text_pose, "Go back to origin position \n Press next to perform motion", rvt::WHITE, rvt::XXLARGE);
  visual_tools.publishTrajectoryLine(my_plan.trajectory_, csda10f_joint_model_group);
  visual_tools.trigger();
  visual_tools.prompt("Press the 'next' button on the 'RvizVisualToolsGui' pannel");

  // Perform motion on real robot
  visual_tools.deleteAllMarkers();
  csda10f_move_group.execute(my_plan);
  ros::Duration(1.5).sleep();

  //************************************************************************************************
  // STEP 6.1: The right gripper open.

  // Use the previously defined home position for ease of motion, this position was defined on the moveit_config package
  right_gripper_move_group.setStartState(*right_gripper_move_group.getCurrentState());
  right_gripper_move_group.setNamedTarget("open_right_gripper");
  success = (right_gripper_move_group.plan(my_plan) == moveit::planning_interface::MoveItErrorCode::SUCCESS);
  // visual_tools.publishText(text_pose, "The right gripper open", rvt::WHITE, rvt::XXLARGE);
  // visual_tools.trigger();
  // visual_tools.prompt("Press the 'next' button on the 'RvizVisualToolsGui' pannel");

  // Perform motion on real robot
  visual_tools.deleteAllMarkers();
  right_gripper_move_group.execute(my_plan);
  ros::Duration(1.5).sleep();

  // *****************************************************************************************
  
  ros::shutdown();
  return 0;
}
