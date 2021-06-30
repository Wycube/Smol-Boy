#ifndef VIDEO_DEVICE_HPP
#define VIDEO_DEVICE_HPP

#include "common/Types.hpp"


namespace sb {

//An abstract interface used for drawing pixels to a window. It allows different frontends to be used, i.e. SDL, OpenGL, SFML, etc.
//Pixels are drawn to an internal buffer, then can be presented / drawn to another buffer used for a window with the present methods,
//either a line at a time (possibly for HBlank) or the whole screen at a time (for VBlank).
//The internal buffer is stored in RGBA as an array of 8-bit unsigned integers.
class VideoDevice {
protected:

    u8 *m_internal_buffer;
    u32 m_internal_width, m_internal_height;

public:

    VideoDevice(u32 width, u32 height);
    ~VideoDevice();

    void clear_screen(u32 color);
    virtual void draw_pixel(u32 pixel, u32 x, u32 y);
    virtual void present_screen() = 0;
};


class NullVideoDevice : public VideoDevice {
public:

    NullVideoDevice(u32 width, u32 height) : VideoDevice(width, height) { }

    void present_screen() override { }
};

} //namespace sb


#endif //VIDEO_DEVICE_HPP