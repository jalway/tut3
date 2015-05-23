#version 440 core // Identifies the version of the shader, this line must be on a separate line from the rest of the shader code

out vec4 color; // Our vec4 color variable containing r, g, b, a

uniform mat4 MVP; // Our uniform Model-View-Projection matrix to modify our position values

uniform sampler2D heightmapTexture;

void main(void)
{
    int i = gl_VertexID;
    int TileIndex = i / 6;						// Which square is it? We need 2 triangles, so 6 vertices per square.
    int VertexInTile = i % 6;	                // 0, 1, 2, 3, 4, 5, 0, 1... Which vertex is it in the square?
    int TriangleInTile = VertexInTile / 3;      // 0, 0, 0, 1, 1, 1, 0, 0... Which triangle is it in the square?
                                                // 0, 1, 2, 1, 2, 3, 0, 1... Indices for a square.
    int Index = VertexInTile - (2 * TriangleInTile);
    vec2 VertexLocation = vec2(
                        Index % 2, 				// x = index % 2
                        (Index / 2) % 2 );		// z = (index / 2) % 2
    vec2 TileLocation = vec2(
                        (TileIndex % 1023),		//   = index % 1023
                        (TileIndex / 1023));	//   = index / 1023 (we don't need a mod here, since we know we are using a constant 1024*1024 tiles)
    VertexLocation += TileLocation;
        // Wow, there sure was a lot of information in that simple incremental count from 0 to 1023*1023*6.
    

    // The sample location for accessing the 2D texture: essentially a 2D 0 to 1 index of where we are sampling.
    vec2 SampleLocation = VertexLocation / 1024;

    // This value now in a range of 0 to 1, where 0 was 0 in our 8 bit texture and 1 was 255 in our 8 bit texture.
    float Height = texture( heightmapTexture, SampleLocation ).r;
    float OriginalValue = Height;

    // Scale the height up, since we want some interesting elevation.
    Height *= 500.0;

    // Shrink the whole thing down to a more manageable scale. Alternatively, this could be done with the model transformation matrix.
    float ScaleDown = 0.005;
    Height = Height * ScaleDown;
    VertexLocation = VertexLocation * ScaleDown;


    vec4 Position = vec4(VertexLocation.x, Height, VertexLocation.y, 1.0);
    
    vec4 ClipSpace = MVP * Position;
    
    // If on the left side of the screen, draw with a red color based on the value we pulled from the texture.
    if( ClipSpace.x / ClipSpace.w < 0.0 )
    {
        color = vec4(OriginalValue, 0, 0, 1.0);
    }
    else            // If on the right side of the screen, add a bit of interesting effects to it.
    {
        // Recompute the vertex position if it's low; this will give us a water level.
        float WaterLevel = 0.125;
        if( OriginalValue < WaterLevel )
        {
            OriginalValue = WaterLevel;
            Height = OriginalValue * 500.0 * ScaleDown;
            Position = vec4(VertexLocation.x, Height, VertexLocation.y, 1.0);
            ClipSpace = MVP * Position;

            // Give our water a nice blue.
            color = vec4(0.3, 0.5, 0.9, 1.0);
        }
        else
        {
            vec4 ShoreColor = vec4(0.2, 0.23, 0.22, 1.0);
            vec4 GrassColor = vec4(0.05, 0.4, 0.1, 1.0);

            // Figure out how grassy this area should be for a smooth transition. This will lerp from 1.05 * water height (full rocky) to 1.1 (full grassy).
            float GrassinessFactor = clamp((OriginalValue-WaterLevel * 1.05) * 20.0, 0.0, 1.0);
            color = mix(ShoreColor, GrassColor, GrassinessFactor);

            // The golden spire of RIT.
            if(OriginalValue > 0.9)
            {
                color = vec4(1.0, 0.9, 0.0, 1.0);
            }
        }
    }

    // And there you have it: a heightmap of Rochester with a spire in the terrain denoting the position of RIT. (if only the rivers had less of a slope to them and showed up in their entirety)

    gl_Position = ClipSpace;
}													 

