// Copyright (C) 2006 Ruben Smits <ruben dot smits at mech dot kuleuven dot be>
//  
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//  
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//  
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//  

//hardware interfaces
#include <hardware/kuka/Kuka160nAxesVelocityController.hpp>
#include <hardware/kuka/Kuka361nAxesVelocityController.hpp>
#include <hardware/kuka/EmergencyStop.hpp>

//User interface
#include <taskbrowser/TaskBrowser.hpp>

//Reporting
#include <reporting/FileReporting.hpp>

//nAxes components
#include <motion_control/naxes/nAxesComponents.hpp>

#include <rtt/Activities.hpp>
#include <rtt/TaskContext.hpp>
//#include <rtt/Logger.hpp>
#include <rtt/os/main.h>

using namespace Orocos;
using namespace RTT;
using namespace std;

// main() function

int ORO_main(int argc, char* argv[])
{
  TaskContext* my_robot = NULL;
  if (argc > 1)
    {
      string s = argv[1];
      if(s == "Kuka361"){
	log(Warning) <<"Choosing Kuka361"<<endlog();
	my_robot = new Kuka361nAxesVelocityController("Robot");
      }
      else if(s == "Kuka160"){
	log(Warning) <<"Choosing Kuka160"<<endlog();
	my_robot = new Kuka160nAxesVelocityController("Robot");
      }
    }
  else{
    log(Warning) <<"Using Default Kuka361"<<endlog();
    my_robot = new Kuka361nAxesVelocityController("Robot");
  }

  if ( Logger::log().getLogLevel() < Logger::Info ) {
    Logger::log().setLogLevel( Logger::Info );
    log(Info) << argv[0] << " manually raises LogLevel to 'Info' (5)."
		  << " See also file 'orocos.log'."<<endlog();
  }
  
  EmergencyStop _emergency(my_robot);
  
  /// Creating Event Handlers
  _emergency.addEvent(my_robot,"driveOutOfRange");
  _emergency.addEvent(my_robot,"positionOutOfRange");

  //nAxesComponents
  nAxesGeneratorPos generatorPos("nAxesGeneratorPos",6);
  nAxesGeneratorVel generatorVel("nAxesGeneratorVel",6);
  nAxesControllerPos controllerPos("nAxesControllerPos",6);
  nAxesControllerPosVel controllerPosVel("nAxesControllerPosVel",6);
  nAxesControllerVel controllerVel("nAxesControllerVel",6);
  
  //connection naxes components to each other and the robot
  connectPorts(my_robot,&generatorPos);
  connectPorts(my_robot,&generatorVel);
  connectPorts(my_robot,&controllerPos);
  connectPorts(my_robot,&controllerPosVel);
  connectPorts(my_robot,&controllerVel);
  connectPorts(&generatorPos,&controllerPos);
  connectPorts(&generatorPos,&controllerPosVel);
  connectPorts(&generatorVel,&controllerVel);
  connectPorts(&controllerPos,my_robot);
  connectPorts(&controllerPosVel,my_robot);
  connectPorts(&controllerVel,my_robot);
    
  //Reporting
  FileReporting reporter("Reporting");
  connectPorts(&reporter,my_robot);
  connectPorts(&reporter,&generatorPos);
  connectPorts(&reporter,&generatorVel);
  connectPorts(&reporter,&controllerPos);
  connectPorts(&reporter,&controllerPosVel);
  connectPorts(&reporter,&controllerVel);

  //Create supervising TaskContext
  TaskContext super("nAxes");
  
  // Link components to supervisor
  super.addPeer(my_robot);
  super.addPeer(&reporter);
  super.addPeer(&generatorPos); 
  super.addPeer(&generatorVel); 
  super.addPeer(&controllerPos);
  super.addPeer(&controllerPosVel);
  super.addPeer(&controllerVel);
  
  // Load programs in supervisor
  super.scripting()->loadPrograms("cpf/program_calibrate_offsets.ops");
  super.scripting()->loadPrograms("cpf/program_moveto.ops");
  
  // Load StateMachine in supervisor
  super.scripting()->loadStateMachines("cpf/states.osd");

  // Creating Tasks
  PeriodicActivity _kukaTask(0,0.002, my_robot->engine() ); 
  PeriodicActivity superTask(1,0.002,super.engine());
  PeriodicActivity _generatorPosTask(0,0.01, generatorPos.engine() ); 
  PeriodicActivity _generatorVelTask(0,0.01, generatorVel.engine() ); 
  PeriodicActivity _controllerPosTask(0,0.01, controllerPos.engine() ); 
  PeriodicActivity _controllerPosVelTask(0,0.01, controllerPosVel.engine() ); 
  PeriodicActivity _controllerVelTask(0,0.01, controllerVel.engine() ); 
  PeriodicActivity reportingTask(2,0.1,reporter.engine());
  
  TaskBrowser browser(&super);
  browser.setColorTheme( TaskBrowser::whitebg );
  
  superTask.start();
  _kukaTask.start();
  
  //Load Reporterconfiguration and start Reporter
  //reporter.load();
  //reportingTask.start();
  
  // Start the console reader.
  browser.loop();
  
  return 0;
}
