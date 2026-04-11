function(stdgpu_determine_thrust_paths STDGPU_OUTPUT_THRUST_PATHS)
    # Clear list before appending flags
    unset(${STDGPU_OUTPUT_THRUST_PATHS})

    find_package(CUDAToolkit QUIET)

    # In CUDA13, torch includes are located under the cccl dir
    set(${STDGPU_OUTPUT_THRUST_PATHS} "${CUDAToolkit_INCLUDE_DIRS};${CUDAToolkit_INCLUDE_DIRS}/cccl")

    # Make output variable visible
    set(${STDGPU_OUTPUT_THRUST_PATHS} ${${STDGPU_OUTPUT_THRUST_PATHS}} PARENT_SCOPE)
endfunction()
