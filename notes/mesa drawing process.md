# mesa drawing process

#### stage 1

```C
glDrawArrays(...);
```

#### stage 2

```c
/**
 * mesa/main/draw.c
 * Called from glDrawArrays when in immediate mode (not display list mode).
 */
void GLAPIENTRY
_mesa_DrawArrays(GLenum mode, GLint start, GLsizei count){
    ...
}
```

#### stage 3

```c
/**
 * mesa/main/draw.c
 * Helper function called by the other DrawArrays() functions below.
 * This is where we handle primitive restart for drawing non-indexed
 * arrays.  If primitive restart is enabled, it typically means
 * splitting one DrawArrays() into two.
 */
static void
_mesa_draw_arrays(struct gl_context *ctx, GLenum mode, GLint start,
                  GLsizei count, GLuint numInstances, GLuint baseInstance)
{
   /* OpenGL 4.5 says that primitive restart is ignored with non-indexed
    * draws.
    */
   struct pipe_draw_info info;
   struct pipe_draw_start_count_bias draw;

   info.mode = mode;
   info.vertices_per_patch = ctx->TessCtrlProgram.patch_vertices;
   info.index_size = 0;
   /* Packed section begin. */
   info.primitive_restart = false;
   info.has_user_indices = false;
   info.index_bounds_valid = true;
   info.increment_draw_id = false;
   info.take_index_buffer_ownership = false;
   info.index_bias_varies = false;
   /* Packed section end. */
   info.start_instance = baseInstance;
   info.instance_count = numInstances;
   info.view_mask = 0;
   info.min_index = start;
   info.max_index = start + count - 1;

   draw.start = start;
   draw.count = count;

   // core !!!
   ctx->Driver.DrawGallium(ctx, &info, 0, &draw, 1);

   if (MESA_DEBUG_FLAGS & DEBUG_ALWAYS_FLUSH) {
      _mesa_flush(ctx);
   }
}
```

#### stage 4

```c
// driver takes it all
// note that the Driver.DrawGallium is from device driver program


/**
 * mesa/drivers/common/driverfuncs.c
 * Plug in default functions for all pointers in the dd_function_table
 * structure.
 * Device drivers should call this function and then plug in any
 * functions which it wants to override.
 * Some functions (pointers) MUST be implemented by all drivers (REQUIRED).
 *
 * \param table the dd_function_table to initialize
 */
void
_mesa_init_driver_functions(struct dd_function_table *driver)
{
   memset(driver, 0, sizeof(*driver));

   ...

   /* Vertex/fragment programs */
   driver->NewProgram = _mesa_new_program;
   driver->DeleteProgram = _mesa_delete_program;

   /* ATI_fragment_shader */
   driver->NewATIfs = NULL;

   /* Draw functions */
   driver->Draw = NULL;
   driver->DrawGallium = _mesa_draw_gallium_fallback;
   driver->DrawGalliumMultiMode = _mesa_draw_gallium_multimode_fallback;
   driver->DrawIndirect = NULL;
   driver->DrawTransformFeedback = NULL;

   ...
}
```

#### stage 5: what is _mesa_draw_gallium_fallback?

```c
/**
 * mesa/main/draw.c
 * Called via Driver.DrawGallium. This is a fallback invoking Driver.Draw.
 */
void
_mesa_draw_gallium_fallback(struct gl_context *ctx,
                            struct pipe_draw_info *info,
                            unsigned drawid_offset,
                            const struct pipe_draw_start_count_bias *draws,
                            unsigned num_draws)
{
   struct _mesa_index_buffer ib;
   unsigned index_size = info->index_size;
   unsigned min_index = 0, max_index = ~0u;
   bool index_bounds_valid = false;

   ...

   /* Single draw or a fallback for user indices. */
   if (num_draws == 1 ||
       (info->index_size && info->has_user_indices &&
        !ctx->Const.MultiDrawWithUserIndices)) {
      for (unsigned i = 0; i < num_draws; i++) {
         if (!draws[i].count)
            continue;

         if (index_size) {
			...
         }

		 ...

         ctx->Driver.Draw(ctx, &prim, 1, index_size ? &ib : NULL,
                          index_bounds_valid, info->primitive_restart,
                          info->restart_index, min_index, max_index,
                          info->instance_count, info->start_instance);
      }
      return;
   }

   ...

   ctx->Driver.Draw(ctx, prim, num_prims, index_size ? &ib : NULL,
                    index_bounds_valid, info->primitive_restart,
                    info->restart_index, min_index, max_index,
                    info->instance_count, info->start_instance);
   FREE_PRIMS(prim, num_draws);
}
```

#### stage 6: so who defines the Driver.Draw then?

```c
// mesa also writes a bunch of driver functions
// mesa/dirvers/dri/i965/brw_draw.c
void
brw_init_draw_functions(struct dd_function_table *functions)
{
   /* Register our drawing function:
    */
   functions->Draw = brw_draw_prims;
   functions->DrawTransformFeedback = brw_draw_transform_feedback;
   functions->DrawIndirect = brw_draw_indirect_prims;
}
```

