/*
 *  Copyright 2011, 2012, DFKI GmbH Robotics Innovation Center
 *
 *  This file is part of the MARS simulation framework.
 *
 *  MARS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation, either version 3
 *  of the License, or (at your option) any later version.
 *
 *  MARS is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with MARS.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * \file Simulator.h
 * \author Malte Römmermann
 * \brief "Simulator" is the main class of the simulation
 *
 */

#ifndef SIMULATOR_H
#define SIMULATOR_H

#ifdef _PRINT_HEADER_
  #warning "Simulator.h"
#endif

#include <mars/data_broker/DataPackage.h>
#include <mars/data_broker/ReceiverInterface.h>
#include <mars/cfg_manager/CFGManagerInterface.h>
#include <mars/utils/Thread.h>
#include <mars/utils/Mutex.h>
#include <mars/utils/WaitCondition.h>
#include <mars/utils/ReadWriteLock.h>
#include <mars/interfaces/sim/SimulatorInterface.h>
#include <mars/interfaces/sim/PhysicsInterface.h>
#include <mars/interfaces/sim/PluginInterface.h>
#include <mars/interfaces/sim/ControlCenter.h>
#include <mars/interfaces/graphics/GraphicsUpdateInterface.h>

#include <iostream>


namespace mars {
  namespace sim {

    /**
     *\brief The Simulator class implements the main functions of the simulator.
     *
     * Its constructor presents the core function in the simulation and takes directly the arguments
     * given from the user.(if the simulation is starting from a command line).
     * It inherits the mars::Thread Class and the i_GuiToSim
     * A Simulator object presents a separate thread and shares data with all other threads within the process.(remarque )
     * To handles and acces the data  of the Simulator properly the Mutex variable coreMutex are used.
     *
     */
    class Simulator : public utils::Thread,
                      public interfaces::SimulatorInterface,
                      public interfaces::GraphicsUpdateInterface,
                      public lib_manager::LibInterface,
                      public cfg_manager::CFGClient,
                      public data_broker::ReceiverInterface {

    public:

      enum Status { UNKNOWN = -1, STOPPED = 0, RUNNING = 1, STOPPING = 2, STEPPING=3 };

      Simulator(lib_manager::LibManager *theManager);
      virtual ~Simulator();

      // LibInterface methods
      int getLibVersion() const {return 1;}
      const std::string getLibName() const {return std::string("mars_sim");}
      void newLibLoaded(const std::string &libName);
      CREATE_MODULE_INFO();

      void checkOptionalDependency(const std::string &libName);
      void runSimulation();
  
      virtual void receiveData(const data_broker::DataInfo &info,
                               const data_broker::DataPackage &package,
                               int callbackParam);

      virtual void StartSimulation() {
        stepping_mutex.lock();
        simulationStatus = RUNNING;
        stepping_wc.wakeAll();
        stepping_mutex.unlock();
      }
      virtual void StopSimulation() {
        stepping_mutex.lock();
        if(simulationStatus != STOPPED) {
          simulationStatus = STOPPING;
          //stepping_wc.wakeAll();
        }
        stepping_mutex.unlock();
      }

      virtual bool getAllowDraw(void) {return allow_draw;}
      virtual bool getSyncGraphics(void) {return sync_graphics;}

      static Simulator *activeSimulator;
      ///the Simulator constructor

      //Coming from MainGUI
      /** \brief open the osg window */

      void setSyncThreads(bool value);

      /**
       * \brief updates the osg objects position from the simulation
       * updates the simulation
       */
      void updateSim();
      /**
       * \brief allows the osgWidget to draw a frame
       */
      void allowDraw(void);
 
      virtual void exportScene() const;

      virtual void postGraphicsUpdate(void);

      bool startStopTrigger();

      /// control the realtime calculation
      void myRealTime(void);

      virtual int loadScene(const std::string &filename,
                            const std::string &robotname,bool threadsave=false, bool blocking=false);
      virtual int loadScene(const std::string &filename, 
                            bool wasrunning=false,
                            const std::string &robotname="",bool threadsave=false, bool blocking=false);

      virtual bool allConcurrencysHandeled();
      virtual int saveScene(const std::string &filename, bool wasrunning);
      virtual bool isSimRunning() const;
      virtual bool sceneChanged() const;
      virtual void sceneHasChanged(bool reseted);
      virtual bool hasSimFault() const;

      void addLight(interfaces::LightData light);
      virtual void finishedDraw(void);

      virtual void newWorld(bool clear_all = false);
      virtual void resetSim(void);

      virtual void controlSet(unsigned long id, interfaces::sReal value);
      virtual void physicsThreadLock(void);
      virtual void physicsThreadUnlock(void);
      virtual interfaces::PhysicsInterface* getPhysics(void) const;
      virtual void exitMars(void);
      virtual void connectNodes(unsigned long id1, unsigned long id2);
      virtual void disconnectNodes(unsigned long id1, unsigned long id2);
      virtual void rescaleEnvironment(interfaces::sReal x, interfaces::sReal y, interfaces::sReal z);

      virtual void singleStep(void);
      virtual void switchPluginUpdateMode(int mode, interfaces::PluginInterface *pl);
      virtual void handleError(interfaces::PhysicsError error);
      virtual void setGravity(const utils::Vector &gravity);

      virtual interfaces::ControlCenter* getControlCenter(void) const;
      virtual void addPlugin(const interfaces::pluginStruct& plugin);
      virtual void removePlugin(interfaces::PluginInterface *pl);

      virtual int checkCollisions(void);
      virtual void sendDataToPlugin(int plugin_index, void* data);
      //  virtual double initTimer(void);
      //  virtual double getTimer(double start) const;

      virtual void cfgUpdateProperty(cfg_manager::cfgPropertyStruct _property);

      const std::string getConfigDir() const {return config_dir;}
      void readArguments(int argc, char **argv);

      //public slots:
      // TODO: currently this is disabled
      void noGUITimerUpdate(void);

    private:
      struct LoadOptions{
        std::string filename;
        std::string robotname;
        bool wasRunning;
      };
    
      std::vector<LoadOptions> filesToLoad;
      //This method is used for all Calls that cannot be done from an external thread,
      //this means the requests have to be caches (like in loadScene) and has to be handelt by
      //this methos, which is called by this (run())
      void processRequests();
    
      int loadScene_internal(const std::string &filename, bool wasrunning, const std::string &robotname);
      bool sim_fault;
      bool exit_sim;
      bool allow_draw;
      bool sync_graphics;
      int cameraMenuCheckedIndex;
      Status simulationStatus;

      //IceServer comServer;
      interfaces::ControlCenter *control;
      bool b_SceneChanged;
      bool reloadSim;
      short running;
      char was_running;
      bool kill_sim;
      bool show_time;
      interfaces::sReal sync_time;
      bool my_real_time;
      bool fast_step;
      bool erased_active;
      utils::ReadWriteLock pluginLocker;
      int sync_count;
      utils::Mutex externalMutex;
      utils::Mutex coreMutex;
      utils::Mutex physicsMutex;
      utils::Mutex physicsCountMutex;
      
      /**
       * This mutex is used to to prevent active waiting for an single step or start event
       */
      utils::Mutex stepping_mutex;
      /**
       * This Wait Conition is used to to prevent active waiting for an single step or start event
       */
      utils::WaitCondition stepping_wc;
      
      
      int physics_mutex_count;
      interfaces::PhysicsInterface *physics;
      double calc_ms;
      int load_option;
      int std_port;
      utils::Vector gravity;

      std::vector<interfaces::pluginStruct> allPlugins;
      std::vector<interfaces::pluginStruct> newPlugins;
      std::vector<interfaces::pluginStruct> activePlugins;
      std::vector<interfaces::pluginStruct> guiPlugins;

      std::string scenename;
      std::list<std::string> arg_v_scene_name;
      int arg_actual, arg_no_gui, arg_run, arg_grid, arg_ortho;
      std::string config_dir;

      cfg_manager::cfgPropertyStruct cfgCalcMs, cfgFaststep;
      cfg_manager::cfgPropertyStruct cfgRealtime, cfgDebugTime;
      cfg_manager::cfgPropertyStruct cfgSyncGui, cfgDrawContact;
      cfg_manager::cfgPropertyStruct cfgGX, cfgGY, cfgGZ;
      cfg_manager::cfgPropertyStruct cfgWorldErp, cfgWorldCfm;
      cfg_manager::cfgPropertyStruct cfgVisRep;
      cfg_manager::cfgPropertyStruct cfgSyncTime;

      cfg_manager::cfgPropertyStruct configPath;

      unsigned long dbPhysicsUpdateId;
      unsigned long dbSimTimeId;
      data_broker::DataPackage dbPhysicsUpdatePackage;
      data_broker::DataPackage dbSimTimePackage;

      void reloadWorld(void);
      void initCfgParams(void);

    protected:
      /**
       * \brief The simulator main loop.
       *
       * This function is executing while the program is running.
       * It handles the physical simulation, if the physical simulation is started,
       * otherwise the function is in idle mode.
       *
       * pre:
       *     start the simulator thread and by the way the Physics loop
       *
       * post:
       *
       */
      void run();

    };

  } // end of namespace sim
} // end of namespace mars

#endif  // SIMULATOR_H
