#pragma once
#include "Renderer.h"
#include "core.h"
#include <GLFW/glfw3.h>
#include <core_shader.h>
#include <core_graphics_pipeline.h>
#include <core_simple_mesh.h>
#include <core_glfw.h>
#include <core_rt.h>
#include <core_vertex.h>
#include "utils.h"
#include <iostream>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

class VulkanRenderer : public Renderer {
    VulkanRenderer() {

    }

    ~VulkanRenderer() {
        vkDestroyShaderModule(m_vkcore.GetDevice(), rgen, nullptr);
        vkDestroyShaderModule(m_vkcore.GetDevice(), rmiss, nullptr);
        vkDestroyShaderModule(m_vkcore.GetDevice(), rchit, nullptr);
    }

    /**
     * @brief Initializes the underlying graphics library (GL, Vulkan...)
     * @return true, if succeeded
     */
    virtual bool init() {
        m_pWindow = core::glwf_vulkan_init(1080, 1080, "AppName");
        m_vkcore.Init("AppName", m_pWindow);

        m_device = m_vkcore.GetDevice();
        m_numImages = m_vkcore.GetNumImages();

        m_pQueue = m_vkcore.GetQueue();

        m_renderPass = m_vkcore.CreateSimpleRenderPass();
        m_frameBuffers = m_vkcore.CreateFrameBuffers(m_renderPass);

        m_outTexture = new core::VulkanTexture();

        rgen = core::CreateShaderModuleFromText(m_vkcore.GetDevice(), "shaders/raytrace.rgen");
        rmiss = core::CreateShaderModuleFromText(m_vkcore.GetDevice(), "shaders/raytrace.rmiss");
        rchit = core::CreateShaderModuleFromText(m_vkcore.GetDevice(), "shaders/raytrace.rchit");

        m_raytracer.initRayTracing(m_vkcore.GetSelectedPhysicalDevice(), &m_device);
        m_raytracer.setup(m_vkcore.GetCommandPool(), &m_vkcore);

        m_raytracer.createRtDescriptorSet();
        m_raytracer.createMvpDescriptorSet();

    }


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
        const std::vector<uint32_t> inds) {


    }

    /**
     * @brief Add a previously defined mesh to the scene
     * @param modelMatrix transformation applied to the mesh
     * @param id the mesh id
     * @return false if the mesh id does not exists
     */
    virtual bool addMesh(const glm::mat4& modelMatrix, const glm::vec3
        & color, MeshId id) {
        dirtyupdate = true;

        //En principio actualizar las meshes es escribir de nuevo un uniforme
    }


    virtual TextureId addTexture(uint8_t* texels, uint32_t width,
        uint32_t height, uint32_t bpp) {
        //Para esto usar los pushconstants??
    }

    virtual void deleteTexture(TextureId tid) {

    }

    virtual bool addLight(const glm::mat4& modelMatrix, MeshId id,
        const glm::vec3& color, LightId lid, TextureId tid = 0) {

    }

    /**
     * @brief Removes the indicated mesh from memory (and all its
instances in the scene)
      * @param id
      * @return false if the mesh id does not exists
      */
    virtual bool removeMesh(MeshId id) {
        dirtyupdate = true;
    }

    /**
     * @brief removes all the meshes from the scene
     */
    virtual void clearScene() {
        dirtyupdate = true;
    }

    /**
     * @brief Defines the camera
     * @param viewMatrix
     * @param projMatrix
     */
    virtual void setCamera(const glm::mat4& viewMatrix, const
        glm::mat4& projMatrix) {
        VP = projMatrix * viewMatrix;
        //Updatear el buffer que esta en el descriptor set
        m_raytracer.UpdateMvpMatrix(VP);
    }

    /**
     * @brief Defines the size of the result image
     * @param width
     * @param height
     */
    virtual void setOutputResolution(uint32_t width, uint32_t height) {
        windowwidth = width;
        windowheight = height;
        m_raytracer.createOutImage(windowwidth, windowheight, m_outTexture);
    }

    /**
     * @brief Renders the scene. The result can be obtained with Renderer::copyResultBytes or Renderer::getResultTextureId
      * @return
      */
    virtual bool render() {
        //Antes de entregar quitar ek guardar en png
        m_raytracer.render(windowwidth, windowheight, true, "Test1.png");
        return true;
    }

    /**
     * @brief  Copies the final image into buffer
     * @param buffer destination
     * @param bufferSize size of buffer
     * @return the number of bytes written to buffer
     */
    virtual size_t copyResultBytes(uint8_t* buffer, size_t bufferSize) {
        return m_vkcore.copyResultBytes(buffer, bufferSize, m_outTexture, windowwidth, windowheight);
    }

    /*
     * @brief Returns the texture object id with the result image
     * @return the GL? object Id (0 if there is not a texture, or it is
     *   not compatible with GL)
     */
    virtual uint32_t getResultTextureId() {
        //Mirar el interop
    }

    private:
        core::VulkanQueue* m_pQueue;
        core::VulkanCore m_vkcore;
        VkDevice m_device = NULL;
        int m_numImages = 0;
        std::vector<VkCommandBuffer> m_cmdBufs;
        GLFWwindow* m_pWindow;
        VkRenderPass m_renderPass;
        std::vector<VkFramebuffer> m_frameBuffers;

        VkShaderModule rgen, rmiss, rchit;

        
        core::VulkanTexture* m_outTexture;

        uint32_t windowwidth, windowheight;

        /////meshes
        bool dirtyupdate = false;


        glm::mat4 VP;

        ///RAYTRACING
        core::Raytracer m_raytracer;
};