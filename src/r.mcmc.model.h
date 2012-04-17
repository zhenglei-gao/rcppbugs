///////////////////////////////////////////////////////////////////////////
// Copyright (C) 2011 Whit Armstrong                                     //
//                                                                       //
// This program is free software: you can redistribute it and/or modify  //
// it under the terms of the GNU General Public License as published by  //
// the Free Software Foundation, either version 3 of the License, or     //
// (at your option) any later version.                                   //
//                                                                       //
// This program is distributed in the hope that it will be useful,       //
// but WITHOUT ANY WARRANTY; without even the implied warranty of        //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         //
// GNU General Public License for more details.                          //
//                                                                       //
// You should have received a copy of the GNU General Public License     //
// along with this program.  If not, see <http://www.gnu.org/licenses/>. //
///////////////////////////////////////////////////////////////////////////

#ifndef R_MCMC_MODEL_H
#define R_MCMC_MODEL_H

#include <iostream>
#include <cmath>
#include <vector>
#include <map>
#include <exception>
#include <cppbugs/mcmc.object.hpp>
#include <cppbugs/mcmc.stochastic.hpp>
#include "mcmc.rng.hpp"

namespace cppbugs {
  //typedef std::map<void*,MCMCObject*> vmc_map;
  //typedef std::map<void*,MCMCObject*>::iterator vmc_map_iter;

  class RMCModel {
  private:
    double accepted_;
    double rejected_;
    RNativeRng rng_;
    std::vector<MCMCObject*> mcmcObjects_;
    std::vector<MCMCObject*> dynamic_nodes;
    std::vector<Likelihiood*> logp_functors;
    //std::function<void ()> update;
    //vmc_map data_node_map;
   
    void jump() { for(auto v : dynamic_nodes) { v->jump(rng_); } }
    void preserve() { for(auto v : dynamic_nodes) { v->preserve(); } }
    void revert() { for(auto v : dynamic_nodes) { v->revert(); } }
    void set_scale(const double scale) { for(auto v : dynamic_nodes) { v->setScale(scale); } }
    void tally() { for(auto v : dynamic_nodes) { v->tally(); } }
    //void print() { for(auto v : mcmcObjects_) { v->print(); } }
    static bool bad_logp(const double value) { return std::isnan(value) || value == -std::numeric_limits<double>::infinity() ? true : false; }

    void addStochcasticNode(MCMCObject* node) {
      Stochastic* sp = dynamic_cast<Stochastic*>(node);
      // FIXME: this should throw if sp->getLikelihoodFunctor() returns null
      if(sp && sp->getLikelihoodFunctor() ) {
        std::cout << "adding stochastic" << std::endl;
        logp_functors.push_back(sp->getLikelihoodFunctor());
      }
    }

    void initChain() {
      //logp_functors.clear();
      //jumping_nodes.clear();

      for(auto node : mcmcObjects_) {
        // FIXME: add test here to check starting from invalid logp or NaN
        addStochcasticNode(node);

        if(!node->isObserved()) {
          dynamic_nodes.push_back(node);
        }
      }
      // init values
      // update();
    }

    bool reject(const double value, const double old_logp) {
      std::cout << "logp diff: " << value - old_logp << std::endl;
      return bad_logp(value) || log(rng_.uniform()) > (value - old_logp) ? true : false;
    }

    void tune(int iterations, int tuning_step) {
      double logp_value,old_logp_value;
      logp_value  = logp();
      old_logp_value = logp_value;

      for(int i = 1; i <= iterations; i++) {
	for(auto it : dynamic_nodes) {
          old_logp_value = logp_value;
          it->preserve();
          it->jump(rng_);
          //update();
          logp_value = logp();
          std::cout << "global logp: " << logp_value << std::endl;
          //print();
          //getchar();
          if(reject(logp_value, old_logp_value)) {
            std::cout << "reverting" << std::endl;
            it->revert();
            logp_value = old_logp_value;
            it->reject();
          } else {
            std::cout << "accepting" << std::endl;
            it->accept();
          }
	}
	if(i % tuning_step == 0) {
          //std::cout << "tuning at step: " << i << std::endl;
	  for(auto it : dynamic_nodes) {
	    it->tune();
	  }
	}
        tally();
      }
    }

    void run(int iterations, int burn, int thin) {
      double logp_value,old_logp_value;
      logp_value  = logp();
      old_logp_value = logp_value;
      for(int i = 1; i <= (iterations + burn); i++) {
        //std::cout << i << std::endl;
        old_logp_value = logp_value;
        preserve();
        jump();
        //print();
        //getchar();
        //update();
        logp_value = logp();
        std::cout << "global logp: " << logp_value << std::endl;
        if(reject(logp_value, old_logp_value)) {
          std::cout << "reverting" << std::endl;
          revert();
          logp_value = old_logp_value;
          rejected_ += 1;
        } else {
          std::cout << "accepting" << std::endl;
          accepted_ += 1;
        }
        if(i > burn && (i % thin == 0)) {
          tally();
        }
      }
    }

  public:
    // MCModel(std::function<void ()> update_): accepted_(0), rejected_(0), update(update_) {}
    // FIXME: use generic iteratros later...
    RMCModel(std::vector<MCMCObject*> mcmcObjects): accepted_(0), rejected_(0), mcmcObjects_(mcmcObjects) {
      initChain();
    }
    
    /*
    ~MCModel() {
      // use data_node_map as delete list
      // only objects allocated by this class are inserted there
      // addNode allows user allocated objects to enter the mcmcObjects vector
      for(auto m : data_node_map) {
        delete m.second;
      }
    }
    */

    double acceptance_ratio() const {
      return accepted_ / (accepted_ + rejected_);
    }


    double logp() const {
      double ans(0);
      int i = 1;
      for(auto f : logp_functors) {
        //ans += f->calc();
        std::cout << i << ": " << f->calc() << std::endl; ++i;
        ans += f->calc();
      }
      return ans;
    }

    void sample(int iterations, int burn, int adapt, int thin) {
      if(iterations % thin) {
        throw std::logic_error("ERROR: interations not a multiple of thin.");
      }
      // tuning phase
      //std::cout  << "tuning" << std::endl;
      //tune(adapt,static_cast<int>(adapt/100));

      std::cout  << "running" << std::endl;
      // sampling
      run(iterations, burn, thin);
    }

    /*
    template<template<typename> class MCTYPE, typename T>
    MCTYPE<T>& track(T& x) {
      MCTYPE<T>* node = new MCTYPE<T>(x);
      mcmcObjects.push_back(node);
      data_node_map[(void*)(&x)] = node;
      return *node;
    }

    template<template<typename> class MCTYPE, typename T>
    MCTYPE<T>& track(const T& x) {
      MCTYPE<T>* node = new MCTYPE<T>(x);
      mcmcObjects.push_back(node);
      data_node_map[(void*)(&x)] = node;
      return *node;
    }


    // allows node to be added without being put on the delete list
    // for those who want full control of their memory...
    void track(MCMCObject* node) {
      mcmcObjects.push_back(node);
    }

    template<typename T>
    MCMCSpecialized<T>& getNode(const T& x) {
      vmc_map_iter iter = data_node_map.find((void*)(&x));
      if(iter == data_node_map.end()) {
        throw std::logic_error("node not found.");
      }
      MCMCSpecialized<T>* ans = dynamic_cast<MCMCSpecialized<T>*>(iter->second);
      if(ans == nullptr) {
        throw std::logic_error("invalid node conversion.");
      }
      return *ans;
    }
    */
  };
} // namespace cppbugs
#endif // R_MCMC_MODEL_H
