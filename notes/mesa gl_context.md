# mesa gl_context

## struct definition of context

```C
// mtypes.h
struct gl_context
{
   /** State possibly shared with other contexts in the address space */
   struct gl_shared_state *Shared;

   /** Whether Shared->BufferObjects has already been locked for this context. */
   bool BufferObjectsLocked;
   /** Whether Shared->TexMutex has already been locked for this context. */
   bool TexturesLocked;

   /** \name API function pointer tables */
   /*@{*/
   gl_api API;

   /**
    * The current dispatch table for non-displaylist-saving execution, either
    * BeginEnd or OutsideBeginEnd
    */
   struct _glapi_table *Exec;
   /**
    * The normal dispatch table for non-displaylist-saving, non-begin/end
    */
   struct _glapi_table *OutsideBeginEnd;
   /** The dispatch table used between glNewList() and glEndList() */
   struct _glapi_table *Save;
   /**
    * The dispatch table used between glBegin() and glEnd() (outside of a
    * display list).  Only valid functions between those two are set, which is
    * mostly just the set in a GLvertexformat struct.
    */
   struct _glapi_table *BeginEnd;
   /**
    * Dispatch table for when a graphics reset has happened.
    */
   struct _glapi_table *ContextLost;
   /**
    * Dispatch table used to marshal API calls from the client program to a
    * separate server thread.  NULL if API calls are not being marshalled to
    * another thread.
    */
   struct _glapi_table *MarshalExec;
   /**
    * Dispatch table currently in use for fielding API calls from the client
    * program.  If API calls are being marshalled to another thread, this ==
    * MarshalExec.  Otherwise it == CurrentServerDispatch.
    */
   struct _glapi_table *CurrentClientDispatch;

   /**
    * Dispatch table currently in use for performing API calls.  == Save or
    * Exec.
    */
   struct _glapi_table *CurrentServerDispatch;

   /*@}*/

   struct glthread_state GLThread;

   struct gl_config Visual;
   struct gl_framebuffer *DrawBuffer;	/**< buffer for writing */
   struct gl_framebuffer *ReadBuffer;	/**< buffer for reading */
   struct gl_framebuffer *WinSysDrawBuffer;  /**< set with MakeCurrent */
   struct gl_framebuffer *WinSysReadBuffer;  /**< set with MakeCurrent */

   /**
    * Device driver function pointer table
    */
   struct dd_function_table Driver;

   /** Core/Driver constants */
   struct gl_constants Const;

   /**
    * Bitmask of valid primitive types supported by this context type,
    * GL version, and extensions, not taking current states into account.
    * Current states can further reduce the final bitmask at draw time.
    */
   GLbitfield SupportedPrimMask;

   /**
    * Bitmask of valid primitive types depending on current states (such as
    * shaders). This is 0 if the current states should result in
    * GL_INVALID_OPERATION in draw calls.
    */
   GLbitfield ValidPrimMask;

   GLenum16 DrawGLError; /**< GL error to return from draw calls */

   /**
    * Same as ValidPrimMask, but should be applied to glDrawElements*.
    */
   GLbitfield ValidPrimMaskIndexed;

   /**
    * Whether DrawPixels/CopyPixels/Bitmap are valid to render.
    */
   bool DrawPixValid;

   /** \name The various 4x4 matrix stacks */
   /*@{*/
   struct gl_matrix_stack ModelviewMatrixStack;
   struct gl_matrix_stack ProjectionMatrixStack;
   struct gl_matrix_stack TextureMatrixStack[MAX_TEXTURE_UNITS];
   struct gl_matrix_stack ProgramMatrixStack[MAX_PROGRAM_MATRICES];
   struct gl_matrix_stack *CurrentStack; /**< Points to one of the above stacks */
   /*@}*/

   /** Combined modelview and projection matrix */
   GLmatrix _ModelProjectMatrix;

   /** \name Display lists */
   struct gl_dlist_state ListState;

   GLboolean ExecuteFlag;	/**< Execute GL commands? */
   GLboolean CompileFlag;	/**< Compile GL commands into display list? */

   /** Extension information */
   struct gl_extensions Extensions;

   /** GL version integer, for example 31 for GL 3.1, or 20 for GLES 2.0. */
   GLuint Version;
   char *VersionString;

   /** \name State attribute stack (for glPush/PopAttrib) */
   /*@{*/
   GLuint AttribStackDepth;
   struct gl_attrib_node *AttribStack[MAX_ATTRIB_STACK_DEPTH];
   /*@}*/

   /** \name Renderer attribute groups
    *
    * We define a struct for each attribute group to make pushing and popping
    * attributes easy.  Also it's a good organization.
    */
   /*@{*/
   struct gl_accum_attrib	Accum;		/**< Accum buffer attributes */
   struct gl_colorbuffer_attrib	Color;		/**< Color buffer attributes */
   struct gl_current_attrib	Current;	/**< Current attributes */
   struct gl_depthbuffer_attrib	Depth;		/**< Depth buffer attributes */
   struct gl_eval_attrib	Eval;		/**< Eval attributes */
   struct gl_fog_attrib		Fog;		/**< Fog attributes */
   struct gl_hint_attrib	Hint;		/**< Hint attributes */
   struct gl_light_attrib	Light;		/**< Light attributes */
   struct gl_line_attrib	Line;		/**< Line attributes */
   struct gl_list_attrib	List;		/**< List attributes */
   struct gl_multisample_attrib Multisample;
   struct gl_pixel_attrib	Pixel;		/**< Pixel attributes */
   struct gl_point_attrib	Point;		/**< Point attributes */
   struct gl_polygon_attrib	Polygon;	/**< Polygon attributes */
   GLuint PolygonStipple[32];			/**< Polygon stipple */
   struct gl_scissor_attrib	Scissor;	/**< Scissor attributes */
   struct gl_stencil_attrib	Stencil;	/**< Stencil buffer attributes */
   struct gl_texture_attrib	Texture;	/**< Texture attributes */
   struct gl_transform_attrib	Transform;	/**< Transformation attributes */
   struct gl_viewport_attrib	ViewportArray[MAX_VIEWPORTS];	/**< Viewport attributes */
   GLuint SubpixelPrecisionBias[2];	/**< Viewport attributes */
   /*@}*/

   /** \name Client attribute stack */
   /*@{*/
   GLuint ClientAttribStackDepth;
   struct gl_client_attrib_node ClientAttribStack[MAX_CLIENT_ATTRIB_STACK_DEPTH];
   /*@}*/

   /** \name Client attribute groups */
   /*@{*/
   struct gl_array_attrib	Array;	/**< Vertex arrays */
   struct gl_pixelstore_attrib	Pack;	/**< Pixel packing */
   struct gl_pixelstore_attrib	Unpack;	/**< Pixel unpacking */
   struct gl_pixelstore_attrib	DefaultPacking;	/**< Default params */
   /*@}*/

   /** \name Other assorted state (not pushed/popped on attribute stack) */
   /*@{*/
   struct gl_pixelmaps          PixelMaps;

   struct gl_evaluators EvalMap;   /**< All evaluators */
   struct gl_feedback   Feedback;  /**< Feedback */
   struct gl_selection  Select;    /**< Selection */

   struct gl_program_state Program;  /**< general program state */
   struct gl_vertex_program_state VertexProgram;
   struct gl_fragment_program_state FragmentProgram;
   struct gl_geometry_program_state GeometryProgram;
   struct gl_compute_program_state ComputeProgram;
   struct gl_tess_ctrl_program_state TessCtrlProgram;
   struct gl_tess_eval_program_state TessEvalProgram;
   struct gl_ati_fragment_shader_state ATIFragmentShader;

   struct gl_pipeline_shader_state Pipeline; /**< GLSL pipeline shader object state */
   struct gl_pipeline_object Shader; /**< GLSL shader object state */

   /**
    * Current active shader pipeline state
    *
    * Almost all internal users want ::_Shader instead of ::Shader.  The
    * exceptions are bits of legacy GLSL API that do not know about separate
    * shader objects.
    *
    * If a program is active via \c glUseProgram, this will point to
    * \c ::Shader.
    *
    * If a program pipeline is active via \c glBindProgramPipeline, this will
    * point to \c ::Pipeline.Current.
    *
    * If neither a program nor a program pipeline is active, this will point to
    * \c ::Pipeline.Default.  This ensures that \c ::_Shader will never be
    * \c NULL.
    */
   struct gl_pipeline_object *_Shader;

   /**
    * NIR containing the functions that implement software fp64 support.
    */
   struct nir_shader *SoftFP64;

   struct gl_query_state Query;  /**< occlusion, timer queries */

   struct gl_transform_feedback_state TransformFeedback;

   struct gl_perf_monitor_state PerfMonitor;
   struct gl_perf_query_state PerfQuery;

   struct gl_buffer_object *DrawIndirectBuffer; /** < GL_ARB_draw_indirect */
   struct gl_buffer_object *ParameterBuffer; /** < GL_ARB_indirect_parameters */
   struct gl_buffer_object *DispatchIndirectBuffer; /** < GL_ARB_compute_shader */

   struct gl_buffer_object *CopyReadBuffer; /**< GL_ARB_copy_buffer */
   struct gl_buffer_object *CopyWriteBuffer; /**< GL_ARB_copy_buffer */

   struct gl_buffer_object *QueryBuffer; /**< GL_ARB_query_buffer_object */

   /**
    * Current GL_ARB_uniform_buffer_object binding referenced by
    * GL_UNIFORM_BUFFER target for glBufferData, glMapBuffer, etc.
    */
   struct gl_buffer_object *UniformBuffer;

   /**
    * Current GL_ARB_shader_storage_buffer_object binding referenced by
    * GL_SHADER_STORAGE_BUFFER target for glBufferData, glMapBuffer, etc.
    */
   struct gl_buffer_object *ShaderStorageBuffer;

   /**
    * Array of uniform buffers for GL_ARB_uniform_buffer_object and GL 3.1.
    * This is set up using glBindBufferRange() or glBindBufferBase().  They are
    * associated with uniform blocks by glUniformBlockBinding()'s state in the
    * shader program.
    */
   struct gl_buffer_binding
      UniformBufferBindings[MAX_COMBINED_UNIFORM_BUFFERS];

   /**
    * Array of shader storage buffers for ARB_shader_storage_buffer_object
    * and GL 4.3. This is set up using glBindBufferRange() or
    * glBindBufferBase().  They are associated with shader storage blocks by
    * glShaderStorageBlockBinding()'s state in the shader program.
    */
   struct gl_buffer_binding
      ShaderStorageBufferBindings[MAX_COMBINED_SHADER_STORAGE_BUFFERS];

   /**
    * Object currently associated with the GL_ATOMIC_COUNTER_BUFFER
    * target.
    */
   struct gl_buffer_object *AtomicBuffer;

   /**
    * Object currently associated w/ the GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD
    * target.
    */
   struct gl_buffer_object *ExternalVirtualMemoryBuffer;

   /**
    * Array of atomic counter buffer binding points.
    */
   struct gl_buffer_binding
      AtomicBufferBindings[MAX_COMBINED_ATOMIC_BUFFERS];

   /**
    * Array of image units for ARB_shader_image_load_store.
    */
   struct gl_image_unit ImageUnits[MAX_IMAGE_UNITS];

   struct gl_subroutine_index_binding SubroutineIndex[MESA_SHADER_STAGES];
   /*@}*/

   struct gl_meta_state *Meta;  /**< for "meta" operations */

   /* GL_EXT_framebuffer_object */
   struct gl_renderbuffer *CurrentRenderbuffer;

   GLenum16 ErrorValue;      /**< Last error code */

   /**
    * Recognize and silence repeated error debug messages in buggy apps.
    */
   const char *ErrorDebugFmtString;
   GLuint ErrorDebugCount;

   /* GL_ARB_debug_output/GL_KHR_debug */
   simple_mtx_t DebugMutex;
   struct gl_debug_state *Debug;

   GLenum16 RenderMode;      /**< either GL_RENDER, GL_SELECT, GL_FEEDBACK */
   GLbitfield NewState;      /**< bitwise-or of _NEW_* flags */
   GLbitfield PopAttribState; /**< Updated state since glPushAttrib */
   uint64_t NewDriverState;  /**< bitwise-or of flags from DriverFlags */

   struct gl_driver_flags DriverFlags;

   GLboolean ViewportInitialized;  /**< has viewport size been initialized? */
   GLboolean _AllowDrawOutOfOrder;
   /* Is gl_PrimitiveID unused by the current shaders? */
   bool _PrimitiveIDIsUnused;

   /** \name Derived state */
   GLbitfield _ImageTransferState;/**< bitwise-or of IMAGE_*_BIT flags */
   GLfloat _EyeZDir[3];
   GLfloat _ModelViewInvScale; /* may be for model- or eyespace lighting */
   GLfloat _ModelViewInvScaleEyespace; /* always factor defined in spec */
   GLboolean _NeedEyeCoords;
   GLboolean _ForceEyeCoords;

   GLuint TextureStateTimestamp; /**< detect changes to shared state */

   /** \name For debugging/development only */
   /*@{*/
   GLboolean FirstTimeCurrent;
   /*@}*/

   /**
    * False if this context was created without a config. This is needed
    * because the initial state of glDrawBuffers depends on this
    */
   GLboolean HasConfig;

   GLboolean TextureFormatSupported[MESA_FORMAT_COUNT];

   GLboolean RasterDiscard;  /**< GL_RASTERIZER_DISCARD */
   GLboolean IntelConservativeRasterization; /**< GL_CONSERVATIVE_RASTERIZATION_INTEL */
   GLboolean ConservativeRasterization; /**< GL_CONSERVATIVE_RASTERIZATION_NV */
   GLfloat ConservativeRasterDilate;
   GLenum16 ConservativeRasterMode;

   GLboolean IntelBlackholeRender; /**< GL_INTEL_blackhole_render */

   /** Does glVertexAttrib(0) alias glVertex()? */
   bool _AttribZeroAliasesVertex;

   /**
    * When set, TileRasterOrderIncreasingX/Y control the order that a tiled
    * renderer's tiles should be excecuted, to meet the requirements of
    * GL_MESA_tile_raster_order.
    */
   GLboolean TileRasterOrderFixed;
   GLboolean TileRasterOrderIncreasingX;
   GLboolean TileRasterOrderIncreasingY;

   /**
    * \name Hooks for module contexts.
    *
    * These will eventually live in the driver or elsewhere.
    */
   /*@{*/
   void *swrast_context;
   void *swsetup_context;
   void *swtnl_context;
   struct vbo_context vbo_context;
   struct st_context *st;
   /*@}*/

   /**
    * \name NV_vdpau_interop
    */
   /*@{*/
   const void *vdpDevice;
   const void *vdpGetProcAddress;
   struct set *vdpSurfaces;
   /*@}*/

   /**
    * Has this context observed a GPU reset in any context in the share group?
    *
    * Once this field becomes true, it is never reset to false.
    */
   GLboolean ShareGroupReset;

   /**
    * \name OES_primitive_bounding_box
    *
    * Stores the arguments to glPrimitiveBoundingBox
    */
   GLfloat PrimitiveBoundingBox[8];

   struct disk_cache *Cache;

   /**
    * \name GL_ARB_bindless_texture
    */
   /*@{*/
   struct hash_table_u64 *ResidentTextureHandles;
   struct hash_table_u64 *ResidentImageHandles;
   /*@}*/

   bool shader_builtin_ref;
};
```

*gl_context contains all the info. and data needed for gl core functions to render, buffers, attributes and so many config info. are stored*

#### GET_CURRENT_CONTEXT(C) 

```C
// glapi.h
// global pointer 
_GLAPI_EXPORT extern void *_glapi_Context;

// GET_CURRENT_CONTEXT(C), the C means context
#define GET_CURRENT_CONTEXT(C)  struct gl_context *C = (struct gl_context *) \
     (likely(_glapi_Context) ? _glapi_Context : _glapi_get_context())
```

#### who defines the current context

```c
// context.c

// context set: _glapi_set_context((void *) newCtx);
// follow this function, the global pointer _glapi_Context get defined 

/**
 * Bind the given context to the given drawBuffer and readBuffer and
 * make it the current context for the calling thread.
 * We'll render into the drawBuffer and read pixels from the
 * readBuffer (i.e. glRead/CopyPixels, glCopyTexImage, etc).
 *
 * We check that the context's and framebuffer's visuals are compatible
 * and return immediately if they're not.
 *
 * \param newCtx  the new GL context. If NULL then there will be no current GL
 *                context.
 * \param drawBuffer  the drawing framebuffer
 * \param readBuffer  the reading framebuffer
 */
GLboolean
_mesa_make_current( struct gl_context *newCtx,
                    struct gl_framebuffer *drawBuffer,
                    struct gl_framebuffer *readBuffer )
{
   GET_CURRENT_CONTEXT(curCtx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(newCtx, "_mesa_make_current()\n");

   /* Check that the context's and framebuffer's visuals are compatible.
    */
   if (newCtx && drawBuffer && newCtx->WinSysDrawBuffer != drawBuffer) {
      if (!check_compatible(newCtx, drawBuffer)) {
         _mesa_warning(newCtx,
              "MakeCurrent: incompatible visuals for context and drawbuffer");
         return GL_FALSE;
      }
   }
   if (newCtx && readBuffer && newCtx->WinSysReadBuffer != readBuffer) {
      if (!check_compatible(newCtx, readBuffer)) {
         _mesa_warning(newCtx,
              "MakeCurrent: incompatible visuals for context and readbuffer");
         return GL_FALSE;
      }
   }

   if (curCtx &&
       (curCtx->WinSysDrawBuffer || curCtx->WinSysReadBuffer) &&
       /* make sure this context is valid for flushing */
       curCtx != newCtx &&
       curCtx->Const.ContextReleaseBehavior ==
       GL_CONTEXT_RELEASE_BEHAVIOR_FLUSH) {
      _mesa_flush(curCtx);
   }

   /* Call this periodically to detect when the user has begun using
    * GL rendering from multiple threads.
    */
   _glapi_check_multithread();

   if (!newCtx) {
      _glapi_set_dispatch(NULL);  /* none current */
      /* We need old ctx to correctly release Draw/ReadBuffer
       * and avoid a surface leak in st_renderbuffer_delete.
       * Therefore, first drop buffers then set new ctx to NULL.
       */
      if (curCtx) {
         _mesa_reference_framebuffer(&curCtx->WinSysDrawBuffer, NULL);
         _mesa_reference_framebuffer(&curCtx->WinSysReadBuffer, NULL);
      }
      _glapi_set_context(NULL);
      assert(_mesa_get_current_context() == NULL);
   }
   else {
      _glapi_set_context((void *) newCtx);
      assert(_mesa_get_current_context() == newCtx);
      _glapi_set_dispatch(newCtx->CurrentClientDispatch);

      if (drawBuffer && readBuffer) {
         assert(_mesa_is_winsys_fbo(drawBuffer));
         assert(_mesa_is_winsys_fbo(readBuffer));
         _mesa_reference_framebuffer(&newCtx->WinSysDrawBuffer, drawBuffer);
         _mesa_reference_framebuffer(&newCtx->WinSysReadBuffer, readBuffer);

         /*
          * Only set the context's Draw/ReadBuffer fields if they're NULL
          * or not bound to a user-created FBO.
          */
         if (!newCtx->DrawBuffer || _mesa_is_winsys_fbo(newCtx->DrawBuffer)) {
            _mesa_reference_framebuffer(&newCtx->DrawBuffer, drawBuffer);
            /* Update the FBO's list of drawbuffers/renderbuffers.
             * For winsys FBOs this comes from the GL state (which may have
             * changed since the last time this FBO was bound).
             */
            _mesa_update_draw_buffers(newCtx);
            _mesa_update_allow_draw_out_of_order(newCtx);
            _mesa_update_valid_to_render_state(newCtx);
         }
         if (!newCtx->ReadBuffer || _mesa_is_winsys_fbo(newCtx->ReadBuffer)) {
            _mesa_reference_framebuffer(&newCtx->ReadBuffer, readBuffer);
            /* In _mesa_initialize_window_framebuffer, for single-buffered
             * visuals, the ColorReadBuffer is set to be GL_FRONT, even with
             * GLES contexts. When calling read_buffer, we verify we are reading
             * from GL_BACK in is_legal_es3_readbuffer_enum.  But the default is
             * incorrect, and certain dEQP tests check this.  So fix it here.
             */
            if (_mesa_is_gles(newCtx) &&
               !newCtx->ReadBuffer->Visual.doubleBufferMode)
               if (newCtx->ReadBuffer->ColorReadBuffer == GL_FRONT)
                  newCtx->ReadBuffer->ColorReadBuffer = GL_BACK;
         }

         /* XXX only set this flag if we're really changing the draw/read
          * framebuffer bindings.
          */
         newCtx->NewState |= _NEW_BUFFERS;

         check_init_viewport(newCtx, drawBuffer->Width, drawBuffer->Height);
      }

      if (newCtx->FirstTimeCurrent) {
         handle_first_current(newCtx);
         newCtx->FirstTimeCurrent = GL_FALSE;
      }
   }

   return GL_TRUE;
}
```

#### caller of _mesa_make_current

```c
// device related caller uses _mesa_make_current
// AMD radeon
radeonMakeCurrent(...);
// nouveau: a free and open-source graphics device driver
nouveau_context_make_current(...);
// Intel
intelMakeCurrent(...);
```

#### caller of those xxxMakeCurrent

```c
// general function table 
// mesa/drivers/dri/i915/intel_screen.c
static const struct __DriverAPIRec i915_driver_api = {
   .InitScreen		 = intelInitScreen2,
   .DestroyScreen	 = intelDestroyScreen,
   .CreateContext	 = intelCreateContext,
   .DestroyContext	 = intelDestroyContext,
   .CreateBuffer	 = intelCreateBuffer,
   .DestroyBuffer	 = intelDestroyBuffer,
    // general device unrelated MakeCurrent API
   .MakeCurrent		 = intelMakeCurrent,
   .UnbindContext	 = intelUnbindContext,
   .AllocateBuffer       = intelAllocateBuffer,
   .ReleaseBuffer        = intelReleaseBuffer
};
```

#### caller of general interface api.MakeCurrent

```c
// mesa/drivers/x11/glxapi.c
Bool PUBLIC
glXMakeCurrent(Display *dpy, GLXDrawable drawable, GLXContext ctx)
{
   Bool b;
   struct _glxapi_table *t;
   GET_DISPATCH(dpy, t);
   if (!t) {
      return False;
   }
   b = t->MakeCurrent(dpy, drawable, ctx);
   return b;
}
```

*finally, the glXMakeCurrent API is what is usually used for linux programmers*



## gl_buffer_object

```c
// mtypes.h
struct gl_buffer_object
{
   GLint RefCount;
   GLuint Name;
   GLchar *Label;       /**< GL_KHR_debug */

   /**
    * The context that holds a global buffer reference for the lifetime of
    * the GL buffer ID to skip refcounting for all its private bind points.
    * Other contexts must still do refcounting as usual. Shared binding points
    * like TBO within gl_texture_object are always refcounted.
    *
    * Implementation details:
    * - Only the context that creates the buffer ("creating context") skips
    *   refcounting.
    * - Only buffers represented by an OpenGL buffer ID skip refcounting.
    *   Other internal buffers don't. (glthread requires refcounting for
    *   internal buffers, etc.)
    * - glDeleteBuffers removes the global buffer reference and increments
    *   RefCount for all private bind points where the deleted buffer is bound
    *   (e.g. unbound VAOs that are not changed by glDeleteBuffers),
    *   effectively enabling refcounting for that context. This is the main
    *   point where the global buffer reference is removed.
    * - glDeleteBuffers called from a different context adds the buffer into
    *   the ZombieBufferObjects list, which is a way to notify the creating
    *   context that it should remove its global buffer reference to allow
    *   freeing the buffer. The creating context walks over that list in a few
    *   GL functions.
    * - xxxDestroyContext walks over all buffers and removes its global
    *   reference from those buffers that it created.
    */
   struct gl_context *Ctx;
   GLint CtxRefCount;   /**< Non-atomic references held by Ctx. */

   GLenum16 Usage;      /**< GL_STREAM_DRAW_ARB, GL_STREAM_READ_ARB, etc. */
   GLbitfield StorageFlags; /**< GL_MAP_PERSISTENT_BIT, etc. */
   GLsizeiptrARB Size;  /**< Size of buffer storage in bytes */
   
   // data -> array of bytes
   GLubyte *Data;       /**< Location of storage either in RAM or VRAM. */

   GLboolean DeletePending;   /**< true if buffer object is removed from the hash */
   GLboolean Written;   /**< Ever written to? (for debugging) */
   GLboolean Purgeable; /**< Is the buffer purgeable under memory pressure? */
   GLboolean Immutable; /**< GL_ARB_buffer_storage */
   gl_buffer_usage UsageHistory; /**< How has this buffer been used so far? */

   /** Counters used for buffer usage warnings */
   GLuint NumSubDataCalls;
   GLuint NumMapBufferWriteCalls;

   struct gl_buffer_mapping Mappings[MAP_COUNT];

   /** Memoization of min/max index computations for static index buffers */
   simple_mtx_t MinMaxCacheMutex;
   struct hash_table *MinMaxCache;
   unsigned MinMaxCacheHitIndices;
   unsigned MinMaxCacheMissIndices;
   bool MinMaxCacheDirty;

   bool HandleAllocated; /**< GL_ARB_bindless_texture */
};
```

#### new buffer object allocate

```c
// bufferobj.c
static struct gl_buffer_object *
new_gl_buffer_object(struct gl_context *ctx, GLuint id)
{
   struct gl_buffer_object *buf = ctx->Driver.NewBufferObject(ctx, id);

   buf->Ctx = ctx;
   buf->RefCount++; /* global buffer reference held by the context */
   return buf;
}
```
#### backend of glGenBuffers
```c
// bufferobj.c
/**
 * This is the implementation for glGenBuffers and glCreateBuffers. It is not
 * exposed to the rest of Mesa to encourage the use of nameless buffers in
 * driver internals.
 */
static void
create_buffers(struct gl_context *ctx, GLsizei n, GLuint *buffers, bool dsa)
{
   struct gl_buffer_object *buf;

   if (!buffers)
      return;

   /*
    * This must be atomic (generation and allocation of buffer object IDs)
    */
   _mesa_HashLockMaybeLocked(ctx->Shared->BufferObjects,
                             ctx->BufferObjectsLocked);

   _mesa_HashFindFreeKeys(ctx->Shared->BufferObjects, buffers, n);

   /* Insert the ID and pointer into the hash table. If non-DSA, insert a
    * DummyBufferObject.  Otherwise, create a new buffer object and insert
    * it.
    */
   for (int i = 0; i < n; i++) {
      if (dsa) {
         assert(ctx->Driver.NewBufferObject);
         buf = new_gl_buffer_object(ctx, buffers[i]);
         if (!buf) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCreateBuffers");
            _mesa_HashUnlockMaybeLocked(ctx->Shared->BufferObjects,
                                        ctx->BufferObjectsLocked);
            return;
         }
      }
      else
         buf = &DummyBufferObject;

      _mesa_HashInsertLocked(ctx->Shared->BufferObjects, buffers[i], buf, true);
   }

   _mesa_HashUnlockMaybeLocked(ctx->Shared->BufferObjects,
                               ctx->BufferObjectsLocked);
}

```

#### _mesa_HashInsertLocked

```C
// hash.c
void
_mesa_HashInsertLocked(struct _mesa_HashTable *table, GLuint key, void *data,
                       GLboolean isGenName)
{
   _mesa_HashInsert_unlocked(table, key, data);
   if (!isGenName && table->id_alloc)
      util_idalloc_reserve(table->id_alloc, key);
}
```

