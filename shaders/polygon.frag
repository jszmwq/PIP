#version 430

out vec4 fragColor;
flat in int col;

void main(){    

    fragColor = vec4(1,0,float(col),1);

}
