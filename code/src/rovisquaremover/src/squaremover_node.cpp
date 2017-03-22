#include "squaremover_node.hpp"
#include <rw/math/Transform3D.hpp>
#include <chrono>         // std::chrono::seconds
MoveRobot::MoveRobot()
: q_desired(6, 0, -1.6, 0, -1.5708, 0, 0)
{
    // Up-down motion
    //toolPositions.push_back(rw::math::Vector3D<double>(0,-0.2, 0.858));
    //toolPositions.push_back(rw::math::Vector3D<double>(0,-0.191, 1.00));

	// Square motion
	/*
	UL:
		x: -0.201
		y: -0.191
		z:  0.933

	UR:
		x: 0.099
		y: -0.191
		z: 0.933

	DR:
		x: 0.099
		y: -0.191
		z:0.783

	DL:
		x: -0.201
		y: -0.191
		z: 0.783
	*/
	this->toolPositions.push_back(rw::math::Vector3D<double>(-0.191,-0.201, 0.933)); // y,x,z - totalt retarderet...
	this->toolPositions.push_back(rw::math::Vector3D<double>(-0.191, 0.099, 0.933)); // y,x,z - totalt retarderet...
	this->toolPositions.push_back(rw::math::Vector3D<double>(-0.191,0.099, 0.783)); // y,x,z - totalt retarderet...
	this->toolPositions.push_back(rw::math::Vector3D<double>(-0.191,-0.201, 0.783)); // y,x,z - totalt retarderet...

	this->Robot = nh_.serviceClient<caros_control_msgs::SerialDeviceMovePtp>("/ur_simple_demo_node/caros_serial_device_service_interface/move_ptp");
    this->sub_robotFeedback = nh_.subscribe("/ur_simple_demo_node/caros_serial_device_service_interface/robot_state", 1, &MoveRobot::RobotFeedbackCallback, this);


    // Auto load workcell
    std::cout << "Loading cell: " << SCENE_FILE << std::endl;
    this->_wc = rw::loaders::WorkCellLoader::Factory::load(SCENE_FILE);
	this->_device = _wc->findDevice("UR1");
	this->_state = _wc->getDefaultState();


	this->ik_solver_ = new rw::invkin::JacobianIKSolver(_device, _state);
	this->ik_solver_->setEnableInterpolation(true);
	this->ik_solver_->setInterpolatorStep(0.001);
    this->reverse_kin_thread = new std::thread(&MoveRobot::runner, this);
}


MoveRobot::~MoveRobot()
{
    this->stop = true;

    this->reverse_kin_thread->join();
}

void MoveRobot::runner()
{
    //Set the inital robot state
    this->state_position_lock.lock();
    this->_device->setQ(this->q_desired, _state);
    SendQ(this->q_desired);
    this->state_position_lock.unlock();

    while(!this->stop)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        this->state_position_lock.lock();
        if(this->in_position == false)
        {   //If we are not in the desired state yet, don't try to find the next
            this->state_position_lock.unlock();
            continue;
        }
        rw::math::Transform3D<double> NewToolPosition(this->toolPositions[this->pos_counter], this->_device.get()->baseTend( this->_state).R());
      	std::vector<rw::math::Q> solutions = this->ik_solver_->solve(NewToolPosition, this->_state);

        std::cout << "Number of solutions found: " <<  solutions.size() << std::endl;
      	if(solutions.size() > 0)
        {
            this->in_position = false;
      		this->q_desired = solutions.front();
      		SendQ(this->q_desired);
      	}
        this->state_position_lock.unlock();
    }
}

void MoveRobot::RobotFeedbackCallback(const caros_control_msgs::RobotState::ConstPtr& data){
    //Try to obtain the lock.
    //If this is not possible, simply abort, as this means that the runner thread is not done calculating yet.
    if(this->state_position_lock.try_lock() == false) return;
    rw::math::Q qcurrent_(6, data->q.data[0], data->q.data[1],data->q.data[2], data->q.data[3],data->q.data[4], data->q.data[5]);
    auto diff = (qcurrent_ - this->q_desired).norm2(); // Difference between actual and desired state
    if(diff < 0.05)
    {   //pretty close...
        this->_device->setQ(this->q_desired, this->_state);
        this->in_position = true;
        this->pos_counter = (this->pos_counter + 1) % this->toolPositions.size();
    }
    this->state_position_lock.unlock(); //lose the lock.
}

void MoveRobot::SendQ(rw::math::Q q)
{
	//beginner_tutorials::AddTwoInts srv;
	caros_control_msgs::SerialDeviceMovePtp srv;

	caros_common_msgs::Q Q;
    Q.data.push_back( q[0] );
    Q.data.push_back( q[1] );
    Q.data.push_back( q[2] );
    Q.data.push_back( q[3] );
    Q.data.push_back( q[4] );
    Q.data.push_back( q[5] );


	srv.request.targets.push_back(Q);

	srv.request.speeds.push_back(0.1);

	srv.request.blends.push_back(0.1);

	if (Robot.call(srv)){
		ROS_INFO("Sum: %ld", (long int)srv.response.success);
	} else	{
		ROS_ERROR("Failed to call service add_two_ints");
	}
}
