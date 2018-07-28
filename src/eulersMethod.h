#include "particles.h"
#include "calcs.h"

// updating position values
void updatePosition(struct particle_t *parts, double change)
{
	parts->pos[0] = parts->pos[0] + change * parts->vel[0];
	parts->pos[1] = parts->pos[1] + change * parts->vel[1];
	parts->pos[2] = parts->pos[2] + change * parts->vel[2];

    // s[parNo][timeInd+1].x = s[parNo][timeInd].x + timeChange * v[parNo][timeInd].x;
    // s[parNo][timeInd+1].y = s[parNo][timeInd].y + timeChange * v[parNo][timeInd].y;
    // s[parNo][timeInd+1].z = s[parNo][timeInd].z + timeChange * v[parNo][timeInd].z;
}

// updating velocity values
void updateVelocity(struct particle_t *part, vec3_t *e_field, vece_t *b_field, double change)
{
	parts->vel[0] = parts->vel[0] + change * 
    v[parNo][timeInd+1].x = v[parNo][timeInd].x + timeChange * Fx(v[parNo][timeInd].x, v[parNo][timeInd].y, v[parNo][timeInd].z, E.x, E.y, E.z, B.x, B.y, B.z);
    v[parNo][timeInd+1].y = v[parNo][timeInd].y + timeChange * Fy(v[parNo][timeInd].x, v[parNo][timeInd].y, v[parNo][timeInd].z, E.x, E.y, E.z, B.x, B.y, B.z);
    v[parNo][timeInd+1].z = v[parNo][timeInd].z + timeChange * Fz(v[parNo][timeInd].x, v[parNo][timeInd].y, v[parNo][timeInd].z, E.x, E.y, E.z, B.x, B.y, B.z);
}
