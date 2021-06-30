#include "VideoDevice.hpp"
#include "common/Defines.hpp"


namespace sb {

VideoDevice::VideoDevice(u32 width, u32 height) : m_internal_width(width), m_internal_height(height) {
    m_internal_buffer = new u8[width * height * 4];
}

VideoDevice::~VideoDevice() {
    delete[] m_internal_buffer;
}

void VideoDevice::clear_screen(u32 color) {
    for(u32 y = 0; y < m_internal_height; y++) {
        for(u32 x = 0; x < m_internal_width; x++) {
            draw_pixel(color, x, y);
        }
    }
}

//Default method, since it doesn't present anything to a window, it just modifies the internal buffer so it doesn't
//have to be overriden.
void VideoDevice::draw_pixel(u32 pixel, u32 x, u32 y) {
    //You have to times by four here on the width and the x value so it skips 4 components every pixel, like it's supposed to.
    u32 ix = x * 4;

    #if SB_ENDIAN == SB_BIG_ENDIAN
    m_internal_buffer[(ix + 0) + y * m_internal_width * 4] = (pixel >> 24) & 0xff; //Red
    m_internal_buffer[(ix + 1) + y * m_internal_width * 4] = (pixel >> 16) & 0xff; //Green
    m_internal_buffer[(ix + 2) + y * m_internal_width * 4] = (pixel >>  8) & 0xff; //Blue
    m_internal_buffer[(ix + 3) + y * m_internal_width * 4] = (pixel >>  0) & 0xff; //Alpha
    #else
    m_internal_buffer[(ix + 0) + y * m_internal_width * 4] = (pixel >>  0) & 0xff; //Alpha
    m_internal_buffer[(ix + 1) + y * m_internal_width * 4] = (pixel >>  8) & 0xff; //Blue
    m_internal_buffer[(ix + 2) + y * m_internal_width * 4] = (pixel >> 16) & 0xff; //Green
    m_internal_buffer[(ix + 3) + y * m_internal_width * 4] = (pixel >> 24) & 0xff; //Red
    #endif

}

} //namespace sb