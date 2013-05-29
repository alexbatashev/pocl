/* OpenCL runtime library: pocl_image_util image utility functions

   Copyright (c) 2012 Timo Viitanen / Tampere University of Technology
   
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:
   
   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.
   
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

#include "pocl_cl.h"
#include "pocl_image_util.h"
#include "assert.h"

void
pocl_get_image_information(cl_channel_order ch_order, 
                           cl_channel_type ch_type,
                           int* channels_out,
                           int* elem_size_out)
{
  if (ch_type == CL_SNORM_INT8 || ch_type == CL_UNORM_INT8 ||
      ch_type == CL_SIGNED_INT8 || ch_type == CL_UNSIGNED_INT8 )
    {
      *elem_size_out = 1; /* 1 byte */
    }
  else if (ch_type == CL_UNSIGNED_INT32 || 
           ch_type == CL_FLOAT || ch_type == CL_UNORM_INT_101010 )
    {
      *elem_size_out = 4; /* 32bit -> 4 bytes */
    }
  else if (ch_type == CL_SNORM_INT16 || ch_type == CL_UNORM_INT16 ||
           ch_type == CL_SIGNED_INT16 || ch_type == CL_UNSIGNED_INT16 ||
           ch_type == CL_UNORM_SHORT_555 || ch_type == CL_UNORM_SHORT_565)
    {
      *elem_size_out = 2; /* 16bit -> 2 bytes */
    }
  
  /* channels TODO: verify num of channels*/
  if (ch_order == CL_RGB || ch_order == CL_RGBx )
    {
      *channels_out = 1;
    }
  else
    {
      *channels_out = 4;
    }
  /*
    
  int host_elem_size;
  if (type == CL_FLOAT)
    host_elem_size=4;
  else if (type==CL_UNORM_INT8)
    host_elem_size=1;
  else
    POCL_ABORT_UNIMPLEMENTED();
  if (elem_size_out != NULL) 
    *elem_size_out = host_elem_size;
    
  int host_channels;
  if (order == CL_RGBA)
    host_channels=4;
  else if (order == CL_R) 
    host_channels=1;
  else
    POCL_ABORT_UNIMPLEMENTED();
  if (channels_out != NULL) 
    *channels_out = host_channels;
*/
}

cl_int
pocl_write_image(cl_mem               image,
		 cl_device_id         device_id,
		 const size_t *       origin_, /*[3]*/
		 const size_t *       region_, /*[3]*/
		 size_t               host_row_pitch,
		 size_t               host_slice_pitch, 
		 const void *         ptr)
{
  if (image == NULL)
    return CL_INVALID_MEM_OBJECT;

  if ((ptr == NULL) ||
      (region_ == NULL))
    return CL_INVALID_VALUE;
    
  int width = image->image_width;
  int height = image->image_height;
  cl_channel_order order = image->image_channel_order;
  cl_channel_type type = image->image_channel_data_type;
    
  size_t dev_elem_size = sizeof(cl_float);
  int dev_channels = 4;
    
  int host_elem_size;
  int host_channels;
  pocl_get_image_information (image->image_channel_order,
                              image->image_channel_data_type, 
                              &host_channels, &host_elem_size);
    
  size_t origin[3] = { origin_[0]*dev_elem_size*dev_channels, origin_[1], origin_[2] };
  size_t region[3] = { region_[0]*dev_elem_size*dev_channels, region_[1], region_[2] };
    
  size_t image_row_pitch = width*dev_elem_size*dev_channels;
  size_t image_slice_pitch = 0;
    
  if ((region[0]*region[1]*region[2] > 0) &&
      (region[0]-1 +
       image_row_pitch * (region[1]-1) +
       image_slice_pitch * (region[2]-1) >= image->size))
    return CL_INVALID_VALUE;
    
  cl_float* temp = malloc( width*height*dev_channels*dev_elem_size );
    
  if (temp == NULL) 
    return CL_OUT_OF_HOST_MEMORY;
    
  int x, y, k;
    
  for (y=0; y<height; y++)
    for (x=0; x<width*dev_channels; x++)
      temp[x+y*width*dev_channels] = 0.f;
    
  for (y=0; y<height; y++)
    {
      for (x=0; x<width; x++)
	{
	  cl_float elem[4]; //TODO 0,0,0,0 for some modes?
          
	  for (k=0; k<host_channels; k++) 
	    {
	      if (type == CL_FLOAT)
		elem[k] = ((float*)ptr)[k+(x+y*width)*host_channels];
	      else if (type==CL_UNORM_INT8) 
		{
		  cl_uchar foo = ((cl_uchar*)ptr)[k+(x+y*width)*host_channels];
		  elem[k] = (float)(foo) * (1.f/255.f);
		}
	      else
		POCL_ABORT_UNIMPLEMENTED();
	    }
          
	  if (order == CL_RGBA) 
	    for (k=0; k<4; k++)
	      temp[(x+y*width)*dev_channels+k] = elem[k];
	  else if (order == CL_R) 
	    {
	      temp[(x+y*width)*dev_channels+0] = elem[0];
	      temp[(x+y*width)*dev_channels+1] = 0.f;
	      temp[(x+y*width)*dev_channels+2] = 0.f;
	      temp[(x+y*width)*dev_channels+3] = 1.f;
	    }
	}
    }
      
    
  device_id->write_rect(device_id->data, temp, 
                        image->device_ptrs[device_id->dev_id],
                        origin, origin, region,
                        image_row_pitch, image_slice_pitch,
                        image_row_pitch, image_slice_pitch);
  
  free (temp);
  return CL_SUCCESS;
}
           
extern cl_int         
pocl_read_image(cl_mem               image,
		cl_device_id         device_id,
		const size_t *       origin_, /*[3]*/
		const size_t *       region_, /*[3]*/
		size_t               host_row_pitch,
		size_t               host_slice_pitch, 
		void *               ptr) 
{
    
  if (image == NULL)
    return CL_INVALID_MEM_OBJECT;

  if ((ptr == NULL) ||
      (region_ == NULL))
    return CL_INVALID_VALUE;
    
  int width = image->image_width;
  int height = image->image_height;
  int dev_elem_size = sizeof(cl_float);/* TODO: needs to check elem type*/
  int dev_channels = 4;
  size_t origin[3] = { origin_[0]*dev_elem_size*dev_channels, origin_[1], origin_[2] };
  size_t region[3] = { region_[0]*dev_elem_size*dev_channels, region_[1], region_[2] };
    
  size_t image_row_pitch = width*dev_elem_size*dev_channels;
  size_t image_slice_pitch = 0;
  int calculaatio = region[0]*image->image_row_pitch*(region[1])+ region[2]*image->image_slice_pitch;

  printf("pocl_read_image: image_row_pitch = %d \n", image->image_row_pitch);
  printf("pocl_read_image: image size = %d calculaatio = %d \n", image->size, calculaatio);

  /*
  if ((region[0]*region[1]*region[2] > 0) &&
      (region[0]-1 +
       image_row_pitch * (region[1]-1) +
       image_slice_pitch * (region[2]-1) >= image->size))
    {
      printf("pocl_read_image: region failered\n");
      return CL_INVALID_VALUE;
    }
  */
  int i, j, k;
  
  cl_channel_order order = image->image_channel_order;
  cl_channel_type type = image->image_channel_data_type;
    
  cl_float* temp = malloc( width*height*dev_channels*dev_elem_size );
    
  if (temp == NULL)
    return CL_OUT_OF_HOST_MEMORY;
      
  int host_channels, host_elem_size;
      
  pocl_get_image_information(image->image_channel_order,
                             image->image_channel_data_type,
                             &host_channels, &host_elem_size);
      
  if (host_row_pitch == 0) {
    host_row_pitch = width*host_channels;
  }
    
  size_t buffer_origin[3] = { 0, 0, 0 };
    
  device_id->read_rect(device_id->data, temp, 
		       image->device_ptrs[device_id->dev_id],
		       origin, origin, region,
		       image_row_pitch, image_slice_pitch,
		       image_row_pitch, image_slice_pitch);
    
  for (j=0; j<height; j++) {
    for (i=0; i<width; i++) {
      cl_float elem[4];
        
      for (k=0; k<4; k++)
        elem[k]=0;
      
      if (order == CL_RGBA) {
        for (k=0; k<4; k++) 
          elem[k] = temp[i*dev_channels + j*width*dev_channels + k];
      } else if (order == CL_R) { // host_channels == 1
        elem[0] = temp[i*dev_channels + j*width*dev_channels + 0];
      }
        
      if (type == CL_UNORM_INT8) 
        { // host_channels == 4
          for (k=0; k<host_channels; k++)
            {
              ((cl_uchar*)ptr)[i*host_channels + j*host_row_pitch + k] 
                = (unsigned char)(255*elem[k]);
            }
        }
      else if (type == CL_FLOAT) 
        {
          for (k=0; k<host_channels; k++)
            {
              POCL_ABORT_UNIMPLEMENTED();
              ((cl_float*)ptr)[i*host_channels + j*host_row_pitch + k] 
                = elem[k];
            }
        }
      else
        POCL_ABORT_UNIMPLEMENTED();
    }
  }
  
  free (temp);
    
  return CL_SUCCESS;
}
  
