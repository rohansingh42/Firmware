/****************************************************************************
 *
 *   Copyright (c) 2013-2017 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file LandDetector.h
 * Land detector interface for multicopter, fixedwing and VTOL implementations.
 *
 * @author Johan Jansen <jnsn.johan@gmail.com>
 * @author Julian Oes <julian@oes.ch>
 * @author Lorenz Meier <lorenz@px4.io>
 */

#pragma once

#include <px4_module.h>
#include <px4_module_params.h>
#include <lib/hysteresis/hysteresis.h>
#include <parameters/param.h>
#include <perf/perf_counter.h>
#include <uORB/Subscription.hpp>
#include <uORB/topics/actuator_armed.h>
#include <uORB/topics/parameter_update.h>
#include <uORB/topics/vehicle_land_detected.h>
#include <px4_work_queue/ScheduledWorkItem.hpp>

using namespace time_literals;

namespace land_detector
{

class LandDetector : public ModuleBase<LandDetector>, ModuleParams, px4::ScheduledWorkItem
{
public:
	enum class LandDetectionState {
		FLYING = 0,
		LANDED = 1,
		FREEFALL = 2,
		GROUND_CONTACT = 3,
		MAYBE_LANDED = 4
	};

	LandDetector();
	virtual ~LandDetector();


	/** @see ModuleBase */
	static int custom_command(int argc, char *argv[])
	{
		return print_usage("unknown command");
	}

	/** @see ModuleBase */
	static int print_usage(const char *reason = nullptr);

	/** @see ModuleBase::print_status() */
	int print_status() override;

	/**
	 * @return current state.
	 */
	LandDetectionState get_state() const { return _state; }

	/**
	 * Get the work queue going.
	 */
	void start();

	static int task_spawn(int argc, char *argv[]);

protected:

	/**
	 * Update parameters.
	 */
	virtual void _update_params() = 0;

	/**
	 * Update uORB topics.
	 */
	virtual void _update_topics() = 0;

	/**
	 * @return true if UAV is in a landed state.
	 */
	virtual bool _get_landed_state() = 0;

	/**
	 * @return true if UAV is in almost landed state
	 */
	virtual bool _get_maybe_landed_state() { return false; }

	/**
	 * @return true if UAV is touching ground but not landed
	 */
	virtual bool _get_ground_contact_state() { return false; }

	/**
	 * @return true if UAV is in free-fall state.
	 */
	virtual bool _get_freefall_state() { return false; }

	/**
	 *  @return maximum altitude that can be reached
	 */
	virtual float _get_max_altitude() = 0;

	/**
	 *  @return true if vehicle could be in ground effect (close to ground)
	 */
	virtual bool _get_ground_effect_state() { return false; }

	/** Run main land detector loop at this interval. */
	static constexpr uint32_t LAND_DETECTOR_UPDATE_INTERVAL = 20_ms;

	orb_advert_t _land_detected_pub{nullptr};
	vehicle_land_detected_s _land_detected{};

	LandDetectionState _state{LandDetectionState::LANDED};

	systemlib::Hysteresis _freefall_hysteresis{false};
	systemlib::Hysteresis _landed_hysteresis{true};
	systemlib::Hysteresis _maybe_landed_hysteresis{true};
	systemlib::Hysteresis _ground_contact_hysteresis{true};
	systemlib::Hysteresis _ground_effect_hysteresis{false};

	actuator_armed_s _arming {};

private:

	void _check_params(bool force = false);

	void Run() override;

	void _update_state();

	void _update_total_flight_time();

	bool _previous_armed_state{false}; ///< stores the previous actuator_armed.armed state

	uint64_t _total_flight_time{0}; ///< in microseconds

	hrt_abstime _takeoff_time{0};

	perf_counter_t _cycle_perf{perf_alloc(PC_ELAPSED, "land_detector_cycle")};

	uORB::Subscription _actuator_armed_sub{ORB_ID(actuator_armed)};
	uORB::Subscription _param_update_sub{ORB_ID(parameter_update)};

	DEFINE_PARAMETERS(
		(ParamInt<px4::params::LND_FLIGHT_T_HI>) _param_total_flight_time_high,
		(ParamInt<px4::params::LND_FLIGHT_T_LO>) _param_total_flight_time_low
	);
};

} // namespace land_detector
