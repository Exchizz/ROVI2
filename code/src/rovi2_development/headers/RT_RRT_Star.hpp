#pragma once
//Implementation of the Real-time RRT Star algorithm, with modifications for a faster initial solution.
//Much of the RRT-Connect code is heavily inspired by the Implementation in Robwork

//Paper for RT-RRT-Star:
//https://mediatech.aalto.fi/~phamalainen/FutureGameAnimation/p113-naderi.pdf


#include <rw/pathplanning/PlannerConstraint.hpp>
#include <rw/pathplanning/QSampler.hpp>
#include <rw/math/Q.hpp>
#include <rwlibs/pathplanners/rrt/RRTQToQPlanner.hpp>
#include <rwlibs/pathplanners/rrt/RRTTree.hpp>
#include <rw/pathplanning/QToQPlanner.hpp>
#include <rw/math/Metric.hpp>
#include <chrono>
#include <queue>
#include <random>
#include "RT_RRTNode.hpp"
#include "RT_RRT_Tree.hpp"
#include "LineSampler.hpp"


uint64_t fac(uint64_t n);
typedef RT_RRTNode<rw::math::Q> RT_Node;
typedef rwlibs::pathplanners::RT_RRT_Tree<rw::math::Q> Tree;
typedef rw::trajectory::QPath Path;

enum ExtendResult { Trapped, Reached, Advanced };


class RRTStruct
{
public:
    RRTStruct(const rw::pathplanning::PlannerConstraint& _constraint,
        rw::pathplanning::QSampler::Ptr _sampler,
        rw::math::QMetric::Ptr _metric)
        :
        constraint(_constraint),
        sampler(_sampler),
        metric(_metric)
    {
        RW_ASSERT(_sampler);
        RW_ASSERT(_metric);
    }

    rw::pathplanning::PlannerConstraint constraint;
    rw::pathplanning::QSampler::Ptr sampler;
    rw::math::QMetric::Ptr metric;
};


class RT_RRT_Star
{
    public:
        RT_RRT_Star(rw::math::Q _q_start, rw::math::Q _q_goal, const rw::pathplanning::PlannerConstraint& constraint,
        	rw::pathplanning::QSampler::Ptr sampler, rw::math::QMetric::Ptr metric, rw::models::Device::Ptr _device, double cloeseness = 0.005);

        //Change the goal position to a new one
        void set_new_goal(rw::math::Q q_newgoal);
        void move_agent(RT_Node *_agent_node);
        void add_node_to_tree(RT_Node &x_new, RT_Node &x_closest, std::vector<RT_Node> &x_near);
        void expand_and_rewire();
        rw::math::Q create_random_node();
        void rewire_random_nodes(double epsilon);
        void rewire_from_tree_root(double epsilon);
        bool add_nodes_to_tree(rw::math::Q &x_new, std::vector<RT_Node *> &X_near);
        std::vector<RT_Node *> find_next_path(std::chrono::milliseconds rrt_time, bool force = false);
        std::vector<RT_Node *> plan_path();
        RT_Node *find_nearest_node(RT_Node &node) { return this->find_nearest_node(node.getValue()); };
        RT_Node *find_nearest_node(const rw::math::Q &x);
        size_t get_size();
        bool found_solution();
        double getEpsilon();
        uint64_t nodes_without_parents();
        void validate_tree_structure();
        void connect_trees();
        ExtendResult growTree(Tree *tree, const rw::math::Q& q);
        ExtendResult extend(Tree *tree, const rw::math::Q& q, RT_Node* qNearNode);
        ExtendResult connect(Tree *tree, const rw::math::Q& q);
        void mergeTrees();
        void propegate_new_agent(RT_Node * node, RT_Node *new_parent);
        RT_Node *get_random_node();
        void force_stop() { this->stop = true;}
        std::vector<RT_Node *> get_all_nodes() { return this->startTree._nodes; }
        RT_Node *split_edge_with_point(rw::math::Q point, RT_Node *parent, RT_Node *child);
        RT_Node *get_agent(){return this->agent;}
        RT_Node *get_goal(){return this->goal;}

    private:
        bool force = false;
        double alpha = 0.1;
        double beta = 3;
        double k_max = 1000;
        double r_s = 1;
        double rrt_connect_epsilon = 1;
        bool stop = false;
        std::queue<RT_Node *> Q_r;
        std::queue<RT_Node *> Q_s;
        RRTStruct _rrt;
        RT_Node *goal;
        RT_Node *agent;
        Tree startTree;
        Tree goalTree;
        Tree *TreeA;
        Tree *TreeB;

        uint64_t connect_count = 0;
        std::chrono::steady_clock::time_point rewire_expand_deadline;
        std::chrono::steady_clock::time_point rewire_from_root_deadline;
        double closeness;

        std::default_random_engine generator;
        rw::models::Device::Ptr device;
        std::uniform_real_distribution<double> unit_distribution;
        RT_Node *closest = nullptr;
        double u_X;
};

void saveTreePlot(RT_RRT_Star &rt, std::vector<RT_Node *> goal_path = std::vector<RT_Node *>());
