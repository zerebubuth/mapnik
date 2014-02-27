#include <mapnik/blur.hpp>

#include "agg_blur.h"
#include "agg_pixfmt_rgba.h"
#include <iostream>

namespace agg
{
    //=======================================================stack_blur_rgba32
    template<class Img> 
    void _stack_blur_rgba32(Img& img, unsigned rx, unsigned ry, unsigned offset_x, unsigned offset_y)
    {
        typedef typename Img::color_type color_type;
        typedef typename Img::order_type order_type;
        enum order_e 
        { 
            R = order_type::R, 
            G = order_type::G, 
            B = order_type::B,
            A = order_type::A 
        };

        unsigned x, y, xp, yp, i;
        unsigned stack_ptr;
        unsigned stack_start;

        const int8u* src_pix_ptr;
              int8u* dst_pix_ptr;
        color_type*  stack_pix_ptr;

        unsigned sum_r;
        unsigned sum_g;
        unsigned sum_b;
        unsigned sum_a;
        unsigned sum_in_r;
        unsigned sum_in_g;
        unsigned sum_in_b;
        unsigned sum_in_a;
        unsigned sum_out_r;
        unsigned sum_out_g;
        unsigned sum_out_b;
        unsigned sum_out_a;
        unsigned w   = img.width();
        unsigned h   = img.height();
        unsigned wm  = w - (offset_x*2)- 1;
        unsigned hm  = h - (offset_y*2) - 1;


        unsigned div;
        unsigned mul_sum;
        unsigned shr_sum;

        pod_vector<color_type> stack;

        if(rx > 0)
        {
            if(rx > 254) rx = 254;
            div = rx * 2 + 1;
            mul_sum = stack_blur_tables<int>::g_stack_blur8_mul[rx];
            shr_sum = stack_blur_tables<int>::g_stack_blur8_shr[rx];
            stack.allocate(div);

            for(y = offset_y; y < (h-offset_y); y++)
            {
                sum_r = 
                sum_g = 
                sum_b = 
                sum_a = 
                sum_in_r = 
                sum_in_g = 
                sum_in_b = 
                sum_in_a = 
                sum_out_r = 
                sum_out_g = 
                sum_out_b = 
                sum_out_a = 0;

                src_pix_ptr = img.pix_ptr(offset_y, y);
                for(i = 0; i <= rx; i++)
                {
                    stack_pix_ptr    = &stack[i];
                    stack_pix_ptr->r = src_pix_ptr[R];
                    stack_pix_ptr->g = src_pix_ptr[G];
                    stack_pix_ptr->b = src_pix_ptr[B];
                    stack_pix_ptr->a = src_pix_ptr[A];
                    sum_r           += src_pix_ptr[R] * (i + 1);
                    sum_g           += src_pix_ptr[G] * (i + 1);
                    sum_b           += src_pix_ptr[B] * (i + 1);
                    sum_a           += src_pix_ptr[A] * (i + 1);
                    sum_out_r       += src_pix_ptr[R];
                    sum_out_g       += src_pix_ptr[G];
                    sum_out_b       += src_pix_ptr[B];
                    sum_out_a       += src_pix_ptr[A];
                }
                for(i = 1; i <= rx; i++)
                {
                    if(i <= wm) src_pix_ptr += Img::pix_width; 
                    stack_pix_ptr = &stack[i + rx];
                    stack_pix_ptr->r = src_pix_ptr[R];
                    stack_pix_ptr->g = src_pix_ptr[G];
                    stack_pix_ptr->b = src_pix_ptr[B];
                    stack_pix_ptr->a = src_pix_ptr[A];
                    sum_r           += src_pix_ptr[R] * (rx + 1 - i);
                    sum_g           += src_pix_ptr[G] * (rx + 1 - i);
                    sum_b           += src_pix_ptr[B] * (rx + 1 - i);
                    sum_a           += src_pix_ptr[A] * (rx + 1 - i);
                    sum_in_r        += src_pix_ptr[R];
                    sum_in_g        += src_pix_ptr[G];
                    sum_in_b        += src_pix_ptr[B];
                    sum_in_a        += src_pix_ptr[A];
                }

                stack_ptr = rx;
                xp = rx;
                if(xp > wm) xp = wm;
                src_pix_ptr = img.pix_ptr(xp+offset_x, y);
                dst_pix_ptr = img.pix_ptr(offset_x, y);
                for(x = offset_x; x < (w-offset_x); x++)
                {
                    dst_pix_ptr[R] = (sum_r * mul_sum) >> shr_sum;
                    dst_pix_ptr[G] = (sum_g * mul_sum) >> shr_sum;
                    dst_pix_ptr[B] = (sum_b * mul_sum) >> shr_sum;
                    dst_pix_ptr[A] = (sum_a * mul_sum) >> shr_sum;
                    dst_pix_ptr += Img::pix_width;

                    sum_r -= sum_out_r;
                    sum_g -= sum_out_g;
                    sum_b -= sum_out_b;
                    sum_a -= sum_out_a;
       
                    stack_start = stack_ptr + div - rx;
                    if(stack_start >= div) stack_start -= div;
                    stack_pix_ptr = &stack[stack_start];

                    sum_out_r -= stack_pix_ptr->r;
                    sum_out_g -= stack_pix_ptr->g;
                    sum_out_b -= stack_pix_ptr->b;
                    sum_out_a -= stack_pix_ptr->a;

                    if(xp < wm) 
                    {
                        src_pix_ptr += Img::pix_width;
                        ++xp;
                    }
        
                    stack_pix_ptr->r = src_pix_ptr[R];
                    stack_pix_ptr->g = src_pix_ptr[G];
                    stack_pix_ptr->b = src_pix_ptr[B];
                    stack_pix_ptr->a = src_pix_ptr[A];
        
                    sum_in_r += src_pix_ptr[R];
                    sum_in_g += src_pix_ptr[G];
                    sum_in_b += src_pix_ptr[B];
                    sum_in_a += src_pix_ptr[A];
                    sum_r    += sum_in_r;
                    sum_g    += sum_in_g;
                    sum_b    += sum_in_b;
                    sum_a    += sum_in_a;
        
                    ++stack_ptr;
                    if(stack_ptr >= div) stack_ptr = 0;
                    stack_pix_ptr = &stack[stack_ptr];

                    sum_out_r += stack_pix_ptr->r;
                    sum_out_g += stack_pix_ptr->g;
                    sum_out_b += stack_pix_ptr->b;
                    sum_out_a += stack_pix_ptr->a;
                    sum_in_r  -= stack_pix_ptr->r;
                    sum_in_g  -= stack_pix_ptr->g;
                    sum_in_b  -= stack_pix_ptr->b;
                    sum_in_a  -= stack_pix_ptr->a;
                }
            }
        }

        if(ry > 0)
        {
            if(ry > 254) ry = 254;
            div = ry * 2 + 1;
            mul_sum = stack_blur_tables<int>::g_stack_blur8_mul[ry];
            shr_sum = stack_blur_tables<int>::g_stack_blur8_shr[ry];
            stack.allocate(div);

            int stride = img.stride();
            for(x = offset_x; x < (w-offset_x); x++)
            {
                sum_r = 
                sum_g = 
                sum_b = 
                sum_a = 
                sum_in_r = 
                sum_in_g = 
                sum_in_b = 
                sum_in_a = 
                sum_out_r = 
                sum_out_g = 
                sum_out_b = 
                sum_out_a = 0;

                src_pix_ptr = img.pix_ptr(x, offset_x);
                for(i = 0; i <= ry; i++)
                {
                    stack_pix_ptr    = &stack[i];
                    stack_pix_ptr->r = src_pix_ptr[R];
                    stack_pix_ptr->g = src_pix_ptr[G];
                    stack_pix_ptr->b = src_pix_ptr[B];
                    stack_pix_ptr->a = src_pix_ptr[A];
                    sum_r           += src_pix_ptr[R] * (i + 1);
                    sum_g           += src_pix_ptr[G] * (i + 1);
                    sum_b           += src_pix_ptr[B] * (i + 1);
                    sum_a           += src_pix_ptr[A] * (i + 1);
                    sum_out_r       += src_pix_ptr[R];
                    sum_out_g       += src_pix_ptr[G];
                    sum_out_b       += src_pix_ptr[B];
                    sum_out_a       += src_pix_ptr[A];
                }
                for(i = 1; i <= ry; i++)
                {
                    if(i <= hm) src_pix_ptr += stride; 
                    stack_pix_ptr = &stack[i + ry];
                    stack_pix_ptr->r = src_pix_ptr[R];
                    stack_pix_ptr->g = src_pix_ptr[G];
                    stack_pix_ptr->b = src_pix_ptr[B];
                    stack_pix_ptr->a = src_pix_ptr[A];
                    sum_r           += src_pix_ptr[R] * (ry + 1 - i);
                    sum_g           += src_pix_ptr[G] * (ry + 1 - i);
                    sum_b           += src_pix_ptr[B] * (ry + 1 - i);
                    sum_a           += src_pix_ptr[A] * (ry + 1 - i);
                    sum_in_r        += src_pix_ptr[R];
                    sum_in_g        += src_pix_ptr[G];
                    sum_in_b        += src_pix_ptr[B];
                    sum_in_a        += src_pix_ptr[A];
                }

                stack_ptr = ry;
                yp = ry;
                if(yp > hm) yp = hm;
                src_pix_ptr = img.pix_ptr(x, yp+offset_y);
                dst_pix_ptr = img.pix_ptr(x, offset_y);
                for(y = ry; y < (h-offset_y); y++)
                {
                    dst_pix_ptr[R] = (sum_r * mul_sum) >> shr_sum;
                    dst_pix_ptr[G] = (sum_g * mul_sum) >> shr_sum;
                    dst_pix_ptr[B] = (sum_b * mul_sum) >> shr_sum;
                    dst_pix_ptr[A] = (sum_a * mul_sum) >> shr_sum;
                    dst_pix_ptr += stride;

                    sum_r -= sum_out_r;
                    sum_g -= sum_out_g;
                    sum_b -= sum_out_b;
                    sum_a -= sum_out_a;
       
                    stack_start = stack_ptr + div - ry;
                    if(stack_start >= div) stack_start -= div;

                    stack_pix_ptr = &stack[stack_start];
                    sum_out_r -= stack_pix_ptr->r;
                    sum_out_g -= stack_pix_ptr->g;
                    sum_out_b -= stack_pix_ptr->b;
                    sum_out_a -= stack_pix_ptr->a;

                    if(yp < hm) 
                    {
                        src_pix_ptr += stride;
                        ++yp;
                    }
        
                    stack_pix_ptr->r = src_pix_ptr[R];
                    stack_pix_ptr->g = src_pix_ptr[G];
                    stack_pix_ptr->b = src_pix_ptr[B];
                    stack_pix_ptr->a = src_pix_ptr[A];
        
                    sum_in_r += src_pix_ptr[R];
                    sum_in_g += src_pix_ptr[G];
                    sum_in_b += src_pix_ptr[B];
                    sum_in_a += src_pix_ptr[A];
                    sum_r    += sum_in_r;
                    sum_g    += sum_in_g;
                    sum_b    += sum_in_b;
                    sum_a    += sum_in_a;
        
                    ++stack_ptr;
                    if(stack_ptr >= div) stack_ptr = 0;
                    stack_pix_ptr = &stack[stack_ptr];

                    sum_out_r += stack_pix_ptr->r;
                    sum_out_g += stack_pix_ptr->g;
                    sum_out_b += stack_pix_ptr->b;
                    sum_out_a += stack_pix_ptr->a;
                    sum_in_r  -= stack_pix_ptr->r;
                    sum_in_g  -= stack_pix_ptr->g;
                    sum_in_b  -= stack_pix_ptr->b;
                    sum_in_a  -= stack_pix_ptr->a;
                }
            }
        }
    }
}

namespace agg_blur {

    void blur(agg::rendering_buffer & buf, unsigned rx, unsigned ry)
    {
        agg::pixfmt_rgba32_pre pixf(buf);
        unsigned offset_x = rx;
        unsigned offset_y = ry;
        agg::_stack_blur_rgba32(pixf,rx,ry,offset_x,offset_y);
        /*
        agg::rendering_buffer buf2;
        agg::pixfmt_rgba32_pre pixf2(buf2);
        pixf2.attach(pixf,-rx,-ry,pixf.width()-rx,pixf.height()-ry);
       // agg::recursive_blur<agg::rgba8, agg::recursive_blur_calc_rgba<> > m_recursive_blur;
       // m_recursive_blur.blur(pixf2,rx);
        agg::stack_blur_rgba32(pixf2,rx,ry);
//        std::clog << "pixf.width() " << pixf.width() << " pixf2.width() " << pixf2.width() << "\n";
*/
    }

}