#version 430
layout(location=0) in  vec2 vertex;
flat out int pointid;
void main()
{
    float ndcX = vertex.x*0.02-1;
    float ndcY = vertex.y*0.02-1;
    gl_Position = vec4(ndcX,ndcY,0,1);
    pointid = int(gl_VertexID);
}

