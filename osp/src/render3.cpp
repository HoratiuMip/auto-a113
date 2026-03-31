/**
 * @file: osp/render3.cpp
 * @brief: Implementation file.
 * @details: -
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#define STB_IMAGE_IMPLEMENTATION
#include <a113/osp/render3.hpp>

namespace a113 { namespace imm {

void ren_target_t::bind( void ) {
    glGenTextures  ( 1, &_tex_glidx );
    glBindTexture  ( GL_TEXTURE_2D, _tex_glidx );
    glTexImage2D   ( GL_TEXTURE_2D, 0, GL_RGBA, _r.x, _r.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0 );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glBindTexture  ( GL_TEXTURE_2D, GL_NONE );

    glGenFramebuffers( 1, &_fbo );
    glBindFramebuffer( GL_FRAMEBUFFER, _fbo );

    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _tex_glidx, 0 );

    glGenRenderbuffers   ( 1, &_rbo );
    glBindRenderbuffer   ( GL_RENDERBUFFER, _rbo );
    glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, _r.x, _r.y );
    glBindRenderbuffer   ( GL_RENDERBUFFER, GL_NONE );

    glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _rbo );
    glBindFramebuffer        ( GL_FRAMEBUFFER, GL_NONE );
}

} };
