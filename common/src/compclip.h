#pragma once

// "Soft" clipping that compresses after one point and then hard-clips

#define COMPCLIP(func_name, knee, ratio, limit) 				 \
	static inline float func_name(float input) { 				 \
		if (input > knee) { 									 \
			input = (float)knee + (input - (float)knee) * ratio; \
			if (input > limit) return limit; 					 \
			return input; 										 \
		} 														 \
		if (input < -knee) { 									 \
			input = -(float)knee - (input + (float)knee) * ratio;\
			if (input < -limit) return -limit; 					 \
		} 														 \
		return input; 										     \
	}
