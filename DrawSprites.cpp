#include "DrawSprites.hpp"
#include "ColorTextureProgram.hpp"

#include "gl_errors.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <cstddef>
#include <utility>

// most of this code is very similar to DrawLines.cpp

//All DrawSprites instances share one vertex buffer + vertex array object, made at load time:
// (static so they don't collide with DrawLines' globals of the same name)
static GLuint vertex_buffer = 0;
static GLuint vertex_buffer_for_color_program = 0;

static Load< void > setup_buffers(LoadTagDefault, [](){
    
    // set up vertex buffer
    glGenBuffers(1, &vertex_buffer);

	{ //vertex array mapping buffer for color_program:
		//ask OpenGL to fill vertex_buffer_for_color_program with the name of an unused vertex array object:
		glGenVertexArrays(1, &vertex_buffer_for_color_program);

		//set vertex_buffer_for_color_program as the current vertex array object:
		glBindVertexArray(vertex_buffer_for_color_program);

		//set vertex_buffer as the source of glVertexAttribPointer() commands:
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		//set up the vertex array object to describe arrays of PongMode::Vertex:
		glVertexAttribPointer(
			color_texture_program->Position_vec4, //attribute
			3, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(DrawSprites::Vertex), //stride
			(GLbyte *)0 + offsetof(DrawSprites::Vertex, Position) //offset
		);

		glEnableVertexAttribArray(color_texture_program->Position_vec4);
		//[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]

		glVertexAttribPointer(
			color_texture_program->Color_vec4, //attribute
			4, //size
			GL_UNSIGNED_BYTE, //type
			GL_TRUE, //normalized
			sizeof(DrawSprites::Vertex), //stride
			(GLbyte *)0 + offsetof(DrawSprites::Vertex, Color) //offset
		);
		glEnableVertexAttribArray(color_texture_program->Color_vec4);

        glVertexAttribPointer(
            color_texture_program->TexCoord_vec2, //atribute 
            2, GL_FLOAT, GL_FALSE, // size
            sizeof(DrawSprites::Vertex), //stride
            (GLbyte *)0 + offsetof(DrawSprites::Vertex, TexCoord) // offset
        );
        glEnableVertexAttribArray(color_texture_program->TexCoord_vec2);

		//done referring to vertex_buffer, so unbind it:
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//done setting up vertex array object, so unbind it:
		glBindVertexArray(0);
	}

	GL_ERRORS(); //PARANOIA: make sure nothing strange happened during setup
});


DrawSprites::DrawSprites(SpriteSheet const &sheet_, glm::mat4 const &world_to_clip_) : sheet(sheet_), world_to_clip(world_to_clip_) {

}

void DrawSprites::draw(uint32_t row, uint32_t frame, glm::vec2 position, bool flip_x) {
	// snap to anchor to whole pixel
    glm::vec2 p = glm::round(position);

    // quad corners in world pixels
    glm::uvec2 cell = sheet.info.cell_size;
    float x0 = p.x - float(cell.x / 2); 
    float x1 = x0 + float(cell.x); 
    float y0 = p.y;
    float y1 = y0 + float(cell.y);
    
    glm::vec2 uv_min, uv_max; 
    sheet.frame_uvs(row, frame, &uv_min, &uv_max);

    // mirror by swapping left / right texcoords
    if (flip_x) std::swap(uv_min.x, uv_max.x); 

    //image v points down and world y points up so top corners get uv_min.y 
    glm::u8vec4 white(0xff); 
	Vertex bl{ glm::vec3(x0, y0, 0.0f), white, glm::vec2(uv_min.x, uv_max.y) };
	Vertex br{ glm::vec3(x1, y0, 0.0f), white, glm::vec2(uv_max.x, uv_max.y) };
	Vertex tr{ glm::vec3(x1, y1, 0.0f), white, glm::vec2(uv_max.x, uv_min.y) };
	Vertex tl{ glm::vec3(x0, y1, 0.0f), white, glm::vec2(uv_min.x, uv_min.y) };

	//two triangles:
	attribs.emplace_back(bl); attribs.emplace_back(br); attribs.emplace_back(tr);
	attribs.emplace_back(bl); attribs.emplace_back(tr); attribs.emplace_back(tl);
}

DrawSprites::~DrawSprites() {
	if (attribs.empty()) return;

	//based on DrawLines.cpp:

	//upload vertices to vertex_buffer:
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //set vertex_buffer as current
	glBufferData(GL_ARRAY_BUFFER, attribs.size() * sizeof(attribs[0]), attribs.data(), GL_STREAM_DRAW); //upload attribs array
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//set color_program as current program:
	glUseProgram(color_texture_program->program);
	//upload OBJECT_TO_CLIP to the proper uniform location:
	glUniformMatrix4fv(color_texture_program->OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(world_to_clip));

    //sheet texture on unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sheet.texture);

    //straight alpha blending and depth test stuff
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

	//use the mapping vertex_buffer_for_color_program to fetch vertex data:
	glBindVertexArray(vertex_buffer_for_color_program);

	//run the OpenGL pipeline:
	glDrawArrays(GL_TRIANGLES, 0, GLsizei(attribs.size()));

	//reset vertex array to none:
	glBindVertexArray(0);

    //reset texture to 0 
    glBindTexture(GL_TEXTURE_2D, 0);

	//reset current program to none:
	glUseProgram(0);

    //reset blend test
    glDisable(GL_BLEND);

    GL_ERRORS();
}


