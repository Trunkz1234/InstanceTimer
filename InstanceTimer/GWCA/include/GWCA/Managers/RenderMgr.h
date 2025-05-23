#pragma once

#include <GWCA/Utilities/Export.h>

// forward declaration, we don't need to include the full directx header here
struct IDirect3DDevice9;

namespace GW {

    struct Module;
    extern Module RenderModule;

    namespace Render {

        typedef void(__cdecl* RenderCallback) (IDirect3DDevice9*);

        typedef struct Mat4x3f {
            float _11;
            float _12;
            float _13;
            float _14;
            float _21;
            float _22;
            float _23;
            float _24;
            float _31;
            float _32;
            float _33;
            float _34;


            enum Flags {
                Shear = 1 << 3
            };

            uint32_t flags;
        } Mat4x3f;

        enum Transform : int {
            TRANSFORM_PROJECTION_MATRIX = 0,
            TRANSFORM_MODEL_MATRIX = 1,
            // TODO:
            TRANSFORM_COUNT = 5
        };

        // Careful, this doesn't return correct values if you have the U mission map, or other things open
        // prefer to calculate the transformation matrix yourself using Render::GetFieldOfView()
        [[deprecated]] GWCA_API Mat4x3f* GetTransform(Transform transform);

        GWCA_API void EnableHooks();

        // this returns the FoV used for rendering
        GWCA_API float GetFieldOfView();

        // Set up a callback for drawing on screen.
        // Will be called after GW render.
        //
        // Important: if you use this, you should call  GW::Terminate()
        // or at least GW::Render::RestoreHooks() from within the callback
        GWCA_API void SetRenderCallback(RenderCallback callback);

        GWCA_API RenderCallback GetRenderCallback();

        // Set up a callback for directx device reset
        GWCA_API void SetResetCallback(RenderCallback callback);

        // Check if gw is in fullscreen
        // Note: requires one or both callbacks to be set and called before
        // Note: does not update while minimized
        // Note: returns -1 if it doesn't know yet
        GWCA_API int GetIsFullscreen();

        GWCA_API IDirect3DDevice9* GetDevice();

        GWCA_API bool GetIsInRenderLoop();

        GWCA_API uint32_t GetViewportWidth();
        GWCA_API uint32_t GetViewportHeight();
    }
}
