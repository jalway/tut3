#include "glew\glew.h"
#include "glfw\glfw3.h"
#include "glm\glm.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include "glm\gtc\type_ptr.hpp"
#include <iostream>
#include <fstream>
#include <vector>

// This is your reference to your shader program.
// This will be assigned with glCreateProgram().
// This program will run on your GPU.
GLuint program;

// These are your references to your actual compiled shaders
GLuint vertex_shader;
GLuint fragment_shader;

// This is a reference to your uniform MVP matrix in your vertex shader
GLuint uniMVP;
 
// And this is a reference to your texture.
GLuint tex;

// These are 4x4 transformation matrices, which you will locally modify before passing into the vertex shader via uniMVP
glm::mat4 trans;
glm::mat4 proj;
glm::mat4 view;
glm::mat4 MVP;


// This runs once a frame, before renderScene
void update()
{
    static float orbit = 0;
    orbit += 0.0025f;
    const float radius = 3.5f;
    view = glm::lookAt( glm::vec3( sin( orbit )*radius, 2.0f, -cos( orbit )*radius ), glm::vec3( 0.0f, 0.0f, 0.0f ), glm::vec3( 0.0f, 1.0f, 0.0f ) );

	// This takes the value of our transformation, view, and projection matrices and multiplies them together to create the MVP matrix.
	// Then it sets our uniform MVP matrix within our shader to this value.
	// Parameters are: Location within the shader, size (in case we're passing in multiple matrices via a single pointer), whether or not to transpose the matrix, and a pointer 
	// to the matrix value we're passing in.
	MVP = proj * view * trans;
	glUniformMatrix4fv(uniMVP, 1, GL_FALSE, glm::value_ptr(MVP));
}

// This function runs every frame
void renderScene()
{
	// Clear the color buffer and the depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Clear the screen to a neutral gray
	glClearColor(0.5, 0.5, 0.5, 1.0);

	// Tell OpenGL to use the shader program you've created.
    glUseProgram( program );

    // Set the 'slot' we will use for our texture. Unimportant in this case, but you would need these for binding multiple textures to different slots.
    // Texture slots are essentially how you know which texture is which on the GPU, and are implicit based on order.
    glActiveTexture( GL_TEXTURE0 );
    // Bind our texture to this slot. In this demo, it does not actually need to be set every frame, since we have no other textures in our rendering. But if we
    // rendered a different texture from the same slot, we would need to rebind the texture in this slot. Same as with setting the shader program to use.
    // This is an important detail since a lot of switching out shaders and bound resources such as textures can have a high overhead cost. 
    glBindTexture( GL_TEXTURE_2D, tex );

	// Let's draw ~2 million triangles.
    // 1023x1023 tiles; with vertices for each pixel in our image (any more and it wraps, making discontinuities on some of the edges).
	glDrawArrays(GL_TRIANGLES, 0, 6*1023*1023);
}

// This method reads the text from a file.
// Realistically, we wouldn't want plain text shaders hardcoded in, we'd rather read them in from a separate file so that the shader code is separated.
std::string readShader(std::string fileName)
{
	std::string shaderCode;
	std::string line;

	// We choose ifstream and std::ios::in because we are opening the file for input into our program.
	// If we were writing to the file, we would use ofstream and std::ios::out.
	std::ifstream file(fileName, std::ios::in);

	// This checks to make sure that we didn't encounter any errors when getting the file.
	if (!file.good())
	{
		std::cout << "Can't read file: " << fileName.data() << std::endl;

		// Return so we don't error out.
		return "";
	}

	// ifstream keeps an internal "get" position determining the location of the element to be read next
	// seekg allows you to modify this location, and tellg allows you to get this location
	// This location is stored as a streampos member type, and the parameters passed in must be of this type as well
	// seekg parameters are (offset, direction) or you can just use an absolute (position).
	// The offset parameter is of the type streamoff, and the direction is of the type seekdir (an enum which can be ios::beg, ios::cur, or ios::end referring to the beginning, 
	// current position, or end of the stream).
	file.seekg(0, std::ios::end);					// Moves the "get" position to the end of the file.
	shaderCode.resize((unsigned int)file.tellg());	// Resizes the shaderCode string to the size of the file being read, given that tellg will give the current "get" which is at the end of the file.
	file.seekg(0, std::ios::beg);					// Moves the "get" position to the start of the file.

	// File streams contain two member functions for reading and writing binary data (read, write). The read function belongs to ifstream, and the write function belongs to ofstream.
	// The parameters are (memoryBlock, size) where memoryBlock is of type char* and represents the address of an array of bytes are to be read from/written to.
	// The size parameter is an integer that determines the number of characters to be read/written from/to the memory block.
	file.read(&shaderCode[0], shaderCode.size());	// Reads from the file (starting at the "get" position which is currently at the start of the file) and writes that data to the beginning
	// of the shaderCode variable, up until the full size of shaderCode. This is done with binary data, which is why we must ensure that the sizes are all correct.

	file.close(); // Now that we're done, close the file and return the shaderCode.

	return shaderCode;
}

// This method will consolidate some of the shader code we've written to return a GLuint to the compiled shader.
// It only requires the shader source code and the shader type.
GLuint createShader(std::string sourceCode, GLenum shaderType)
{
	// glCreateShader, creates a shader given a type (such as GL_VERTEX_SHADER) and returns a GLuint reference to that shader.
	GLuint shader = glCreateShader(shaderType);
	const char *shader_code_ptr = sourceCode.c_str(); // We establish a pointer to our shader code string
	const int shader_code_size = sourceCode.size();   // And we get the size of that string.

	// glShaderSource replaces the source code in a shader object
	// It takes the reference to the shader (a GLuint), a count of the number of elements in the string array (in case you're passing in multiple strings), a pointer to the string array 
	// that contains your source code, and a size variable determining the length of the array.
	glShaderSource(shader, 1, &shader_code_ptr, &shader_code_size);
	glCompileShader(shader); // This just compiles the shader, given the source code.

	GLint isCompiled = 0;

	// Check the compile status to see if the shader compiled correctly.
	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);

	if (isCompiled == GL_FALSE)
	{
		char infolog[1024];
		glGetShaderInfoLog(shader, 1024, NULL, infolog);

		// Print the compile error.
		std::cout << "The shader failed to compile with the error:" << std::endl << infolog << std::endl;

		// Provide the infolog in whatever manor you deem best.
		// Exit with failure.
		glDeleteShader(shader); // Don't leak the shader.

		// NOTE: I almost always put a break point here, so that instead of the program continuing with a deleted/failed shader, it stops and gives me a chance to look at what may 
		// have gone wrong. You can check the console output to see what the error was, and usually that will point you in the right direction.
	}

	return shader;
}

void initTexture()
{
    // =====================================
    // BEGIN SILLY TEXTURE FILE FORMAT STUFF
    // =====================================

    // Texture files vary depending on their format, but often (especially in games) include some form of compression algorithm
    // in order to reduce their size. This can make decoding them manually somewhat tedious, as you would need to write a
    // decoding algorithm for every variation of file type. File format details can be found online quite easily in most cases,
    // or even reverse-engineered through experimentation if necessary. In this case, I'm just using a plain bitmap texture
    // with properties I know in advance. For most projects, you will use a texture loader like the SOIL library to do loading
    // of textures for you, but it is nonetheless good to know that the reason you do that is because it is tedious rather than
    // because it is magic. The following stuff is a sample of that tedium, and can be replaced with a single line of code
    // using a library like SOIL.

    // Load the file...
    std::ifstream textureFile("../HeightMap.bmp", std::ios::in | std::ios::binary | std::ios::ate );
    int fileSize = (int)textureFile.tellg();
    char* rawTextureFileData = new char[fileSize];
    textureFile.seekg(0, std::ios::beg);
    textureFile.read(rawTextureFileData, fileSize);

    // So what's in it? This: https://en.wikipedia.org/wiki/BMP_file_format

    // Based on that, the first header size is 14 bytes, with the last 4 of those telling how far until the real data starts.
    // So grab that value...
    unsigned int PixelDataOffset;
    memcpy( &PixelDataOffset, &rawTextureFileData[10], 4 );

    // In the next header section, there are 7 possible header formats. This texture happens to be the standard Windows bitmap header.
    // In this particular format, we can find width and height at 18 byte offset and 22 byte offset, both with size of 4 bytes.
    unsigned int Width;
    unsigned int Height;
    memcpy( &Width, &rawTextureFileData[18], 4 );
    memcpy( &Height, &rawTextureFileData[22], 4 );

    // The pixel data starts at this point in the file data. I happen to know this was saved as a 24 bit RGB format.
    unsigned char* rawPixelData = (unsigned char*)&rawTextureFileData[PixelDataOffset];

    // If this were an arbitrary image, we would need even more from the headers; compression details, bits per pixel, and so on.
    // And all this for a relatively simple image format.
    // When it comes down to it, textures files are just like any other file format, they just happen to be relatively well 
    // supported across numerous devices and have a ton of 'standards' and so we take them for granted.

    // With the SOIL library, you could do all that with simply the following:
    // unsigned char* rawPixelData = SOIL_load_image("../Heightmap.bmp", &Width, &Height, 0, SOIL_LOAD_RGB)
    // And it would work with any of the formats it supports. I recommend doing that.

    // =====================================
    // END SILLY TEXTURE FILE FORMAT STUFF
    // =====================================
    

    // However, I also happen to know this texture was saved in a rather wasteful manner: it is a simple greyscale
    // image, and so we really only need 1 color channel to get all its data. GPU texture bandwidth is a valuable
    // commodity for a game engine, so a bit of processing to reduce how much redundant data we sent the GPU
    // can help in some cases. Likewise, less memory use in general is a good thing. So let's reduce it to a single 
    // 8 bit color channel. We will also flip it so 0,0 is in the bottom left (it is top-left in the raw format).
    int numPixels = Width * Height;
    unsigned char* singleChannelPixelData = new unsigned char[numPixels];
    for( unsigned int y = 0; y < Height; y++ )
    {
        for( unsigned int x = 0; x < Width; x++ )
        {
            singleChannelPixelData[y * Width + x] = rawPixelData[((Height - y - 1) * Width + x)*3];
        }
    }
    
    // Put a little spire on the heightmap about where RIT is.
    singleChannelPixelData[270 + 962 * Width] = 255;
    singleChannelPixelData[270 + 962 * Width + 1024] = 255;
    singleChannelPixelData[270 + 962 * Width + 1024 + 1] = 255;
    singleChannelPixelData[270 + 962 * Width + 1] = 255;


    // Now we create our texture object to put this data in: this will then let us send it to the GPU.
    glGenTextures( 1, &tex );

    // Bind the texture: this sets it up so the next calls modify it.
    glBindTexture( GL_TEXTURE_2D, tex );

    // Now we initialize the texture data itself, putting in our pixel values.
    glTexImage2D( GL_TEXTURE_2D, 0, GL_R8, Width, Height, 0, GL_RED, GL_UNSIGNED_BYTE, singleChannelPixelData );

    // If we were doing a standard RGBA 32 bit texture, we would use something like the following:
    // glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, fourChannelPixelData );
    // Or if we set it up with floating point values:
    // glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_FLOAT, fourChannelPixelData );

    // These tell it how it will 'sample' information from the texture in cases where it is either oversampled
    // or undersampled. In this case, we will be using the heightmap as discrete values for vertex locations, so
    // we just want the nearest point to be the sample.
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

    // After we've made our modifications, unbind it so some maroon doesn't come along and change things accidentally.
    glBindTexture( GL_TEXTURE_2D, 0 );
}

// Initialization code
void init()
{
	// Initializes the glew library
	glewInit();

	// Enables the depth test, which you will want in most cases. You can disable this in the render loop if you need to.
	glEnable(GL_DEPTH_TEST);
	

	// Read in the shader code from a file.
	std::string vertShader = readShader("VertexShader.glsl");
	std::string fragShader = readShader("FragmentShader.glsl");

	// createShader consolidates all of the shader compilation code
	vertex_shader = createShader(vertShader, GL_VERTEX_SHADER);
	fragment_shader = createShader(fragShader, GL_FRAGMENT_SHADER);

	// A shader is a program that runs on your GPU instead of your CPU. In this sense, OpenGL refers to your groups of shaders as "programs".
	// Using glCreateProgram creates a shader program and returns a GLuint reference to it.
	program = glCreateProgram();
	glAttachShader(program, vertex_shader);		// This attaches our vertex shader to our program.
	glAttachShader(program, fragment_shader);	// This attaches our fragment shader to our program.

	// This links the program, using the vertex and fragment shaders to create executables to run on the GPU.
	glLinkProgram(program);
	// End of shader and program creation

	// This gets us a reference to the uniform variable in the vertex shader, which is called "MVP".
	// We're using this variable as a 4x4 transformation matrix
	// Only 2 parameters required: A reference to the shader program and the name of the uniform variable within the shader code.
	uniMVP = glGetUniformLocation(program, "MVP");

    // Initialize our texture.
    initTexture();

	// Creates the view matrix using glm::lookAt.
	// First parameter is camera position, second parameter is point to be centered on-screen, and the third paramter is the up axis.
	view = glm::lookAt(	glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	// Creates a projection matrix using glm::perspective.
	// First parameter is the vertical FoV (Field of View), second paramter is the aspect ratio, 3rd parameter is the near clipping plane, 4th parameter is the far clipping plane.
	proj = glm::perspective(45.0f, 800.0f / 600.0f, 0.1f, 1000.0f);

    trans = glm::translate( trans, glm::vec3( -2.5f, -.2f, -2.5f ) );

	// Determines the interpretation of polygons for rasterization. The first parameter, face, determines which polygons the mode applies to.
	// The face can be either GL_FRONT, GL_BACK, or GL_FRONT_AND_BACK
	// The mode determines how the polygons will be rasterized. GL_POINT will draw points at each vertex, GL_LINE will draw lines between the vertices, and 
	// GL_FILL will fill the area inside those lines.
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

int main(int argc, char **argv)
{
	// Initializes the GLFW library
	glfwInit();

	// Creates a window given (width, height, title, monitorPtr, windowPtr).
	// Don't worry about the last two, as they have to do with controlling which monitor to display on and having a reference to other windows. Leaving them as nullptr is fine.
	GLFWwindow* window = glfwCreateWindow(800, 600, "Oh look a heightmap!", nullptr, nullptr);

	// Makes the OpenGL context current for the created window.
	glfwMakeContextCurrent(window);

	// Sets the number of screen updates to wait before swapping the buffers.
	glfwSwapInterval(1);

	// Initializes most things needed before the main loop
	init();

	// Enter the main loop.
	while (!glfwWindowShouldClose(window))
	{
		// Call to the update function; should always be before rendering.
		update();

		// Call the render function.
		renderScene();

		// Swaps the back buffer to the front buffer
		// Remember, you're rendering to the back buffer, then once rendering is complete, you're moving the back buffer to the front so it can be displayed.
		glfwSwapBuffers(window);

		// Checks to see if any events are pending and then processes them.
		glfwPollEvents();
	}

	// After the program is over, cleanup your data!
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	glDeleteProgram(program);
	// Note: If at any point you stop using a "program" or shaders, you should free the data up then and there.


	// Frees up GLFW memory
	glfwTerminate();

	return 0;
}