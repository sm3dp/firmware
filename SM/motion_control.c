

#include "sm_firmware.h"
#include "stepper.h"
#include "planner.h"
#include <math.h>

#define M_PI        3.14159265358979323846
// The arc is approximated by generating a huge number of tiny, linear segments. The length of each
// segment is configured in settings.mm_per_arc_segment.
void mc_arc(float *position, float *target, float *offset, uint8_t axis_0, uint8_t axis_1,
            uint8_t axis_linear, float feed_rate, float radius, uint8_t isclockwise, uint8_t extruder)
{
	//   int acceleration_manager_was_enabled = plan_is_acceleration_manager_enabled();
	//   plan_set_acceleration_manager_enabled(false); // disable acceleration management for the duration of the arc
	float center_axis0 = position[axis_0] + offset[axis_0];
	float center_axis1 = position[axis_1] + offset[axis_1];
	float linear_travel = target[axis_linear] - position[axis_linear];
	float extruder_travel = target[E_AXIS] - position[E_AXIS];
	float r_axis0 = -offset[axis_0];  // Radius vector from center to current location
	float r_axis1 = -offset[axis_1];
	float rt_axis0 = target[axis_0] - center_axis0;
	float rt_axis1 = target[axis_1] - center_axis1;
	float angular_travel;
	float millimeters_of_travel;
	uint16_t segments ;

	float theta_per_segment;
	float linear_per_segment ;
	float extruder_per_segment ;


	float cos_T; // Small angle approximation
	float sin_T;

	float arc_target[4];
	float sin_Ti;
	float cos_Ti;
	float r_axisi;
	uint16_t i;
	int8_t count;

	// CCW angle between position and target from circle center. Only one atan2() trig computation required.
	angular_travel = atan2(r_axis0 * rt_axis1 - r_axis1 * rt_axis0, r_axis0 * rt_axis0 + r_axis1 * rt_axis1);
	if(angular_travel < 0)
	{
		angular_travel += 2 * M_PI;
	}
	if(isclockwise)
	{
		angular_travel -= 2 * M_PI;
	}

	millimeters_of_travel = hypot(angular_travel * radius, fabs(linear_travel));
	if(millimeters_of_travel < 0.001)
	{
		return;
	}
	segments = floor(millimeters_of_travel / MM_PER_ARC_SEGMENT);
	if(segments == 0) segments = 1;

	/*
	  // Multiply inverse feed_rate to compensate for the fact that this movement is approximated
	  // by a number of discrete segments. The inverse feed_rate should be correct for the sum of
	  // all segments.
	  if (invert_feed_rate) { feed_rate *= segments; }
	*/
	theta_per_segment = angular_travel / segments;
	linear_per_segment = linear_travel / segments;
	extruder_per_segment = extruder_travel / segments;

	// Vector rotation matrix values
	cos_T = 1 - 0.5 * theta_per_segment * theta_per_segment; // Small angle approximation
	sin_T = theta_per_segment;
	count = 0;

	// Initialize the linear axis
	arc_target[axis_linear] = position[axis_linear];

	// Initialize the extruder axis
	arc_target[E_AXIS] = position[E_AXIS];

	for(i = 1; i < segments; i++)
	{
		// Increment (segments-1)

		if(count < N_ARC_CORRECTION)
		{
			// Apply vector rotation matrix
			r_axisi = r_axis0 * sin_T + r_axis1 * cos_T;
			r_axis0 = r_axis0 * cos_T - r_axis1 * sin_T;
			r_axis1 = r_axisi;
			count++;
		}
		else
		{
			// Arc correction to radius vector. Computed only every N_ARC_CORRECTION increments.
			// Compute exact location by applying transformation matrix from initial radius vector(=-offset).
			cos_Ti = cos(i * theta_per_segment);
			sin_Ti = sin(i * theta_per_segment);
			r_axis0 = -offset[axis_0] * cos_Ti + offset[axis_1] * sin_Ti;
			r_axis1 = -offset[axis_0] * sin_Ti - offset[axis_1] * cos_Ti;
			count = 0;
		}

		// Update arc_target location
		arc_target[axis_0] = center_axis0 + r_axis0;
		arc_target[axis_1] = center_axis1 + r_axis1;
		arc_target[axis_linear] += linear_per_segment;
		arc_target[E_AXIS] += extruder_per_segment;

		clamp_to_software_endstops(arc_target);
		plan_buffer_line(arc_target[X_AXIS], arc_target[Y_AXIS], arc_target[Z_AXIS], arc_target[E_AXIS], feed_rate, extruder);

	}
	// Ensure last segment arrives at target location.
	plan_buffer_line(target[X_AXIS], target[Y_AXIS], target[Z_AXIS], target[E_AXIS], feed_rate, extruder);

	//   plan_set_acceleration_manager_enabled(acceleration_manager_was_enabled);
}

