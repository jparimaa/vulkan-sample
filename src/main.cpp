#include <cstdio>
#include <vector>
#include <cstdlib>
#include <array>
#include <set>
#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

struct QueueFamilyIndices {
	int graphicsFamily = -1;
	int computeFamily = -1;
	int presentFamily = -1;
};

VkInstance m_instance;
bool m_shouldQuit = false;
VkSurfaceKHR m_surface;
QueueFamilyIndices m_queueFamilyIndices;
VkSwapchainKHR m_swapchain;
std::vector<VkImage> m_swapchainImages;
VkPhysicalDevice m_physicalDevice;
VkDevice m_device;
VkQueue m_graphicsQueue;
VkQueue m_presentQueue;
VkCommandPool m_graphicsCommandPool;
VkCommandBuffer m_commandBuffer;
std::vector<VkSemaphore> m_imageAvailableBinarySemaphores;
std::vector<VkSemaphore> m_renderFinishedBinarySemaphores;
VkFence m_fence;

Display *dis;
int screen;
Window win;
GC gc;

const std::vector<const char*> c_validationLayers = {};
//const std::vector<const char*> c_instanceExtensions{};
// const std::vector<const char*> c_validationLayers = { "VK_LAYER_KHRONOS_validation" };
// const std::vector<const char*> c_instanceExtensions{ VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME , VK_KHR_XLIB_SURFACE_EXTENSION_NAME};
const std::vector<const char*> c_instanceExtensions{ VK_KHR_SURFACE_EXTENSION_NAME , VK_KHR_XLIB_SURFACE_EXTENSION_NAME};
const std::vector<const char*> c_deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
const VkPresentModeKHR c_presentMode = VK_PRESENT_MODE_FIFO_KHR;
const int c_windowWidth = 1848;
const int c_windowHeight = 1016;
const VkSurfaceFormatKHR c_windowSurfaceFormat{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
const VkExtent2D c_windowExtent{ c_windowWidth, c_windowHeight };
const uint32_t c_swapchainImageCount = 3;
const uint64_t c_timeout = 10000000000;

#define VK_CHECK(f)                                                                             \
    do {                                                                                        \
        const VkResult result = (f);                                                            \
        if (result != VK_SUCCESS) {                                                             \
            printf("Abort. %s failed at %s:%d. Result = %d\n", #f, __FILE__, __LINE__, result); \
            abort();                                                                            \
        }                                                                                       \
    } while (false)

#define CHECK(f)                                                           \
    do {                                                                   \
        if (!(f)) {                                                        \
            printf("Abort. %s failed at %s:%d\n", #f, __FILE__, __LINE__); \
            abort();                                                       \
        }                                                                  \
    } while (false)

template <typename T>
uint32_t ui32Size(const T& container)
{
	return static_cast<uint32_t>(container.size());
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type,
	const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
{
	if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		printf("Vulkan warning ");
	}
	else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		printf("Vulkan error ");
	}
	else {
		return VK_FALSE;
	}

	printf("(%d)\n%s\n%s\n\n", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
	return VK_FALSE;
}

bool hasAllQueueFamilies(const QueueFamilyIndices& indices)
{
	return indices.graphicsFamily != -1 && indices.computeFamily != -1 && indices.presentFamily != -1;
}

void getQueueFamilies()
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilies.data());

	QueueFamilyIndices indices;
	for (unsigned int i = 0; i < queueFamilies.size(); ++i) {
		if (queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}

		if (queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
			indices.computeFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, i, m_surface, &presentSupport);
		if (queueFamilies[i].queueCount > 0 && presentSupport) {
			indices.presentFamily = i;
		}

		if (hasAllQueueFamilies(indices)) {
			break;
		}
	}

	m_queueFamilyIndices = indices;
}

void createInstance()
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "MyApp";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;

	std::vector<const char*> instanceExtensions;
	instanceExtensions.insert(instanceExtensions.end(), c_instanceExtensions.begin(), c_instanceExtensions.end());

	VkDebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo{};
	debugUtilsCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugUtilsCreateInfo.messageSeverity =               //
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |  //
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
	debugUtilsCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	debugUtilsCreateInfo.pfnUserCallback = debugUtilsCallback;

	VkInstanceCreateInfo instanceCreateInfo{};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	instanceCreateInfo.enabledExtensionCount = ui32Size(instanceExtensions);
	instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
	instanceCreateInfo.enabledLayerCount = ui32Size(c_validationLayers);
	instanceCreateInfo.ppEnabledLayerNames = c_validationLayers.data();
	instanceCreateInfo.pNext = &debugUtilsCreateInfo;
	instanceCreateInfo.pNext = nullptr;

	VK_CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance));
}

void createWindow()
{
	/* get the colors black and white (see section for details) */
	unsigned long black,white;

	/* use the information from the environment variable DISPLAY
	   to create the X connection:
	*/
	dis=XOpenDisplay((char *)0);
   	screen=DefaultScreen(dis);
	black=BlackPixel(dis,screen),	/* get color black */
	white=WhitePixel(dis, screen);  /* get color white */

	/* once the display is initialized, create the window.
	   This window will be have be 200 pixels across and 300 down.
	   It will have the foreground white and background black
	*/
// XSetWindowAttributes wa;
// wa.override_redirect = True;

   	win=XCreateSimpleWindow(dis,DefaultRootWindow(dis),0,0,		c_windowWidth, c_windowHeight, 5, white, black);

	/* here is where some properties of the window can be set.
	   The third and fourth items indicate the name which appears
	   at the top of the window and the name of the minimized window
	   respectively.
	*/
	XSetStandardProperties(dis,win,"My Window","HI!",None,NULL,0,NULL);

	/* this routine determines which types of input are allowed in
	   the input.  see the appropriate section for details...
	*/
	XSelectInput(dis, win, ExposureMask|ButtonPressMask|KeyPressMask);

	/* create the Graphics Context */
        gc=XCreateGC(dis, win, 0,0);

	/* here is another routine to set the foreground and background
	   colors _currently_ in use in the window.
	*/
	XSetBackground(dis,gc,white);
	XSetForeground(dis,gc,black);

	/* clear the window and bring it on top of the other windows */
	XClearWindow(dis, win);
	XMapRaised(dis, win);
/*
	    Atom wm_state = XInternAtom(dis, "_NET_WM_STATE", False);
    Atom fullscreen = XInternAtom(dis, "_NET_WM_STATE_FULLSCREEN", False);

    XEvent xev;
    memset(&xev, 0, sizeof(xev));
    xev.type = ClientMessage;
    xev.xclient.window = win;
    xev.xclient.message_type = wm_state;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = 1;
    xev.xclient.data.l[1] = fullscreen;
    xev.xclient.data.l[2] = 0;

  XSendEvent (dis, DefaultRootWindow(dis), False,
                    SubstructureRedirectMask | SubstructureNotifyMask, &xev);

*/

    XFlush(dis);



// surface
	VkXlibSurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext =nullptr;
    createInfo.flags = 0;
    createInfo.dpy = dis;
    createInfo.window = win;
	vkCreateXlibSurfaceKHR(m_instance, &createInfo, nullptr, &m_surface);
}

void getPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
	CHECK(deviceCount);

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
	m_physicalDevice = devices[0];
	CHECK(m_physicalDevice != VK_NULL_HANDLE);

	VkPhysicalDeviceProperties m_physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(m_physicalDevice, &m_physicalDeviceProperties);
	printf("GPU: %s\n", m_physicalDeviceProperties.deviceName);
}

void createDevice()
{
	const std::set<int> uniqueQueueFamilies =  //
	{
		m_queueFamilyIndices.graphicsFamily, m_queueFamilyIndices.computeFamily,
		m_queueFamilyIndices.presentFamily  //
	};

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	const float queuePriority = 1.0f;
	for (int queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
	VkPhysicalDeviceVulkan12Features device12Features{};
	device12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pNext = &device12Features;
	createInfo.queueCreateInfoCount = ui32Size(queueCreateInfos);
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = ui32Size(c_deviceExtensions);
	createInfo.ppEnabledExtensionNames = c_deviceExtensions.data();
	createInfo.enabledLayerCount = ui32Size(c_validationLayers);
	createInfo.ppEnabledLayerNames = c_validationLayers.data();

	VK_CHECK(vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device));

	vkGetDeviceQueue(m_device, m_queueFamilyIndices.graphicsFamily, 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_device, m_queueFamilyIndices.presentFamily, 0, &m_presentQueue);
}

void createSwapchain()
{
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_surface;
	createInfo.minImageCount = c_swapchainImageCount;
	createInfo.imageFormat = c_windowSurfaceFormat.format;
	createInfo.imageColorSpace = c_windowSurfaceFormat.colorSpace;
	createInfo.imageExtent = {c_windowWidth,c_windowHeight};
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.queueFamilyIndexCount = 0;
	createInfo.pQueueFamilyIndices = nullptr;
	createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = c_presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VK_CHECK(vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain));

	uint32_t queriedImageCount;
	vkGetSwapchainImagesKHR(m_device, m_swapchain, &queriedImageCount, nullptr);
	CHECK(queriedImageCount == c_swapchainImageCount);
	m_swapchainImages.resize(c_swapchainImageCount);
	vkGetSwapchainImagesKHR(m_device, m_swapchain, &queriedImageCount, m_swapchainImages.data());
}

void createCommandPool()
{
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = m_queueFamilyIndices.graphicsFamily;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VK_CHECK(vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_graphicsCommandPool));
}

void allocateCommandBuffer()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_graphicsCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	VK_CHECK(vkAllocateCommandBuffers(m_device, &allocInfo, &m_commandBuffer));
}

void createSemaphores()
{
	m_imageAvailableBinarySemaphores.resize(c_swapchainImageCount);
	m_renderFinishedBinarySemaphores.resize(c_swapchainImageCount);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (size_t i = 0; i < c_swapchainImageCount; ++i) {
		VK_CHECK(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableBinarySemaphores[i]));
		VK_CHECK(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedBinarySemaphores[i]));
	}
}

void createFence()
{
	VkFenceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	vkCreateFence(m_device, &createInfo, nullptr, &m_fence);
}

void destroyResources()
{
	vkDeviceWaitIdle(m_device);
	vkDestroyFence(m_device, m_fence, nullptr);
	for (size_t i = 0; i < c_swapchainImageCount; ++i) {
		vkDestroySemaphore(m_device, m_imageAvailableBinarySemaphores[i], nullptr);
		vkDestroySemaphore(m_device, m_renderFinishedBinarySemaphores[i], nullptr);
	}
	vkDestroyCommandPool(m_device, m_graphicsCommandPool, nullptr);
	vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
	vkDestroyDevice(m_device, nullptr);
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	XFreeGC(dis, gc);
	XDestroyWindow(dis,win);
	XCloseDisplay(dis);

	vkDestroyInstance(m_instance, nullptr);
}

int main(void)
{
	createInstance();
	createWindow();
	getPhysicalDevice();
	getQueueFamilies();
	createDevice();
	createSwapchain();
	createCommandPool();
	allocateCommandBuffer();
	createSemaphores();
	createFence();

	uint64_t frameIndex = 0;
	std::vector<bool> layoutChanged(ui32Size(m_swapchainImages), false);

	while (true) {
		uint32_t imageIndex;
		//VK_CHECK(vkAcquireNextImageKHR(m_device, m_swapchain, c_timeout, m_imageAvailableBinarySemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex));
		vkAcquireNextImageKHR(m_device, m_swapchain, c_timeout, m_imageAvailableBinarySemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex);
		VK_CHECK(vkWaitForFences(m_device, 1, &m_fence, true, c_timeout));
		VK_CHECK(vkResetFences(m_device, 1, &m_fence));

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		VkCommandBuffer cb = m_commandBuffer;
		vkResetCommandBuffer(cb, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
		vkBeginCommandBuffer(cb, &beginInfo);

		if (!layoutChanged[imageIndex])
		{
			layoutChanged[imageIndex] = true;

			const VkImageSubresourceRange subresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			VkImageMemoryBarrier transferDstBarrier{};
			transferDstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			transferDstBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			transferDstBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			transferDstBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			transferDstBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			transferDstBarrier.image = m_swapchainImages[imageIndex];
			transferDstBarrier.subresourceRange = subresourceRange;
			transferDstBarrier.srcAccessMask = 0;
			transferDstBarrier.dstAccessMask = 0;

			vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &transferDstBarrier);
		}

		VK_CHECK(vkEndCommandBuffer(cb));

		const std::vector<VkPipelineStageFlags> waitStages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.pWaitDstStageMask = waitStages.data();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_commandBuffer;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &m_imageAvailableBinarySemaphores[frameIndex];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &m_renderFinishedBinarySemaphores[frameIndex];

		VK_CHECK(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_fence));

		VkPresentInfoKHR presentInfo{};
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &m_renderFinishedBinarySemaphores[frameIndex];
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_swapchain;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		//VK_CHECK(vkQueuePresentKHR(m_presentQueue, &presentInfo));
		vkQueuePresentKHR(m_presentQueue, &presentInfo);


		++frameIndex;
		if (frameIndex == c_swapchainImageCount) {
			frameIndex = 0;
		}
	}

	destroyResources();

	return 0;
}