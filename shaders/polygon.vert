#version 430
layout(location=0) in vec2 vertex;
layout(location=1) in int id;

flat out int col;

void main()
{
    float ndcX = vertex.x*0.02-1;
    float ndcY = vertex.y*0.02-1;
    gl_Position = vec4(ndcX,ndcY,0,1);
    col = id;
}
