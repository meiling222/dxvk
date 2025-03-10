dxvk_shaders = files([
  'shaders/dxvk_blit_frag_1d.frag',
  'shaders/dxvk_blit_frag_2d.frag',
  'shaders/dxvk_blit_frag_3d.frag',

  'shaders/dxvk_clear_buffer_u.comp',
  'shaders/dxvk_clear_buffer_f.comp',
  'shaders/dxvk_clear_image1d_u.comp',
  'shaders/dxvk_clear_image1d_f.comp',
  'shaders/dxvk_clear_image1darr_u.comp',
  'shaders/dxvk_clear_image1darr_f.comp',
  'shaders/dxvk_clear_image2d_u.comp',
  'shaders/dxvk_clear_image2d_f.comp',
  'shaders/dxvk_clear_image2darr_u.comp',
  'shaders/dxvk_clear_image2darr_f.comp',
  'shaders/dxvk_clear_image3d_u.comp',
  'shaders/dxvk_clear_image3d_f.comp',

  'shaders/dxvk_copy_buffer_image.comp',
  'shaders/dxvk_copy_color_1d.frag',
  'shaders/dxvk_copy_color_2d.frag',
  'shaders/dxvk_copy_color_ms.frag',
  'shaders/dxvk_copy_depth_1d.frag',
  'shaders/dxvk_copy_depth_2d.frag',
  'shaders/dxvk_copy_depth_ms.frag',
  'shaders/dxvk_copy_depth_stencil_1d.frag',
  'shaders/dxvk_copy_depth_stencil_2d.frag',
  'shaders/dxvk_copy_depth_stencil_ms.frag',

  'shaders/dxvk_dummy_frag.frag',

  'shaders/dxvk_fullscreen_geom.geom',
  'shaders/dxvk_fullscreen_vert.vert',
  'shaders/dxvk_fullscreen_layer_vert.vert',

  'shaders/dxvk_pack_d24s8.comp',
  'shaders/dxvk_pack_d32s8.comp',

  'shaders/dxvk_present_frag.frag',
  'shaders/dxvk_present_frag_blit.frag',
  'shaders/dxvk_present_frag_ms.frag',
  'shaders/dxvk_present_frag_ms_amd.frag',
  'shaders/dxvk_present_vert.vert',

  'shaders/dxvk_resolve_frag_d.frag',
  'shaders/dxvk_resolve_frag_ds.frag',
  'shaders/dxvk_resolve_frag_f.frag',
  'shaders/dxvk_resolve_frag_f_amd.frag',
  'shaders/dxvk_resolve_frag_i.frag',
  'shaders/dxvk_resolve_frag_u.frag',

  'shaders/dxvk_unpack_d24s8_as_d32s8.comp',
  'shaders/dxvk_unpack_d24s8.comp',
  'shaders/dxvk_unpack_d32s8.comp',

  'hud/shaders/hud_graph_frag.frag',
  'hud/shaders/hud_graph_vert.vert',

  'hud/shaders/hud_text_frag.frag',
  'hud/shaders/hud_text_vert.vert',
])

dxvk_src = [
  'dxvk_adapter.cpp',
  'dxvk_barrier.cpp',
  'dxvk_buffer.cpp',
  'dxvk_cmdlist.cpp',
  'dxvk_compute.cpp',
  'dxvk_context.cpp',
  'dxvk_cs.cpp',
  'dxvk_data.cpp',
  'dxvk_descriptor.cpp',
  'dxvk_device.cpp',
  'dxvk_device_filter.cpp',
  'dxvk_extensions.cpp',
  'dxvk_fence.cpp',
  'dxvk_format.cpp',
  'dxvk_framebuffer.cpp',
  'dxvk_gpu_event.cpp',
  'dxvk_gpu_query.cpp',
  'dxvk_graphics.cpp',
  'dxvk_image.cpp',
  'dxvk_instance.cpp',
  'dxvk_lifetime.cpp',
  'dxvk_memory.cpp',
  'dxvk_meta_blit.cpp',
  'dxvk_meta_clear.cpp',
  'dxvk_meta_copy.cpp',
  'dxvk_meta_mipgen.cpp',
  'dxvk_meta_pack.cpp',
  'dxvk_meta_resolve.cpp',
  'dxvk_options.cpp',
  'dxvk_pipelayout.cpp',
  'dxvk_pipemanager.cpp',
  'dxvk_queue.cpp',
  'dxvk_resource.cpp',
  'dxvk_sampler.cpp',
  'dxvk_shader.cpp',
  'dxvk_shader_key.cpp',
  'dxvk_signal.cpp',
  'dxvk_staging.cpp',
  'dxvk_state_cache.cpp',
  'dxvk_stats.cpp',
  'dxvk_swapchain_blitter.cpp',
  'dxvk_unbound.cpp',
  'dxvk_util.cpp',

  'hud/dxvk_hud.cpp',
  'hud/dxvk_hud_font.cpp',
  'hud/dxvk_hud_item.cpp',
  'hud/dxvk_hud_renderer.cpp',
]

if platform == 'windows'
  dxvk_src += [
    'dxvk_openvr.cpp',
    'dxvk_openxr.cpp',
  ]
endif

if dxvk_wsi == 'win32'
  dxvk_src += [
    'platform/dxvk_win32_exts.cpp'
  ]
elif dxvk_wsi == 'sdl2'
  dxvk_src += [
    'platform/dxvk_sdl2_exts.cpp'
  ]
endif

thread_dep = dependency('threads')

dxvk_lib = static_library('dxvk', dxvk_src, glsl_generator.process(dxvk_shaders), dxvk_version,
  link_with           : [ util_lib, spirv_lib, wsi_lib ],
  dependencies        : [ thread_dep, vkcommon_dep ],
  include_directories : [ dxvk_include_path ],
)

dxvk_dep = declare_dependency(
  link_with           : [ dxvk_lib ],
  include_directories : [ dxvk_include_path ],
)
