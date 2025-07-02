#pragma once

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <glm/ext.hpp>

class Renderer {
public:
    using MeshId = uint32_t;
    using TextureId = uint32_t;
    using LightId = uint32_t;

    Renderer();
    /**
     * @brief Initializes the underlying graphics library (GL, Vulkan...)
     * @return true, if succeeded
     */
    virtual bool init();
    /**
     * @brief Defines a mesh (it won't be rendered until it is added to
the scene with Renderer::addMesh
      * @param vtcs vertices
      * @param nrmls normals
      * @param uv texture coordinates (optional)
      * @param inds indices
      * @return the MeshId used to add a copy of this mesh to the scene
      */
    virtual MeshId defineMesh(
        const std::vector<glm::vec3>& vtcs,
        const std::vector<glm::vec3>& nrmls,
        const std::vector<glm::vec2>& uv,
        const std::vector<uint32_t> inds);

    /**
     * @brief Add a previously defined mesh to the scene
     * @param modelMatrix transformation applied to the mesh
     * @param id the mesh id
     * @return false if the mesh id does not exists
     */
    virtual bool addMesh(const glm::mat4& modelMatrix, const glm::vec3
        & color, MeshId id);


    virtual TextureId addTexture(uint8_t* texels, uint32_t width,
        uint32_t height, uint32_t bpp);

    virtual deleteTexture(TextureId tid);

    virtual bool addLight(const glm::mat4& modelMatrix, MeshId id,
        const glm::vec3& color, LightId lid, TextureId tid = 0);

    /**
     * @brief Removes the indicated mesh from memory (and all its
instances in the scene)
      * @param id
      * @return false if the mesh id does not exists
      */
    virtual bool removeMesh(MeshId id);

    /**
     * @brief removes all the meshes from the scene
     */
    virtual void clearScene();

    /**
     * @brief Defines the camera
     * @param viewMatrix
     * @param projMatrix
     */
    virtual void setCamera(const glm::mat4& viewMatrix, const
        glm::mat4& projMatrix);

    /**
     * @brief Defines the size of the result image
     * @param width
     * @param height
     */
    virtual void setOutputResolution(uint32_t width, uint32_t height);

    /**
     * @brief Renders the scene. The result can be obtained with
Renderer::copyResultBytes or Renderer::getResultTextureId
      * @return
      */
    virtual bool render();

    /**
     * @brief  Copies the final image into buffer
     * @param buffer destination
     * @param bufferSize size of buffer
     * @return the number of bytes written to buffer
     */
    virtual size_t copyResultBytes(uint8_t* buffer, size_t bufferSize);

    /**
     * @brief Returns the texture object id with the result image
     * @return the GL? object Id (0 if there is not a texture, or it is
not compatible with GL)
      */
    virtual uint32_t getResultTextureId();
};