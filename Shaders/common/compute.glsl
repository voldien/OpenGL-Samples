#ifndef _COMMON_COMPUTE_
#define _COMMON_COMPUTE_ 1

uint getWorkIndex() {

	return (gl_WorkGroupID.z * gl_NumWorkGroups.x * gl_NumWorkGroups.y) + (gl_WorkGroupID.y * gl_NumWorkGroups.x) +
		gl_WorkGroupID.x;
}

uint getIndex() {
	const uint instance_index = gl_GlobalInvocationID.z * (gl_NumWorkGroups.x * gl_NumWorkGroups.y * gl_WorkGroupSize.x * gl_WorkGroupSize.y) +
		gl_GlobalInvocationID.y * (gl_NumWorkGroups.x * gl_WorkGroupSize.x) + gl_GlobalInvocationID.x;

	return instance_index;
}

#endif
