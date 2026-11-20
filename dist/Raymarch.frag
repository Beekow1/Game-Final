#version 330

const int MAX_BLOCKERS = 128;
const int MAX_LIGHTS = 8;

const float HIT_EPSILON = 0.16;

const float light_levels = 6.0;

const vec4 AMBIENT = vec4(0.1, 0.1, 0.1, 1.0);

uniform int BLOCKER_COUNT;
uniform vec4 BLOCKER_BBS[MAX_BLOCKERS];

struct Light {
    vec2 position;
    vec3 color;
    float radius;
    float intensity;
    bool do_march;
};

uniform int LIGHT_COUNT;
uniform Light LIGHTS[MAX_LIGHTS];

out vec4 fragColor;

float sdSquare(vec2 p, vec2 min_p, vec2 max_p) {
    vec2 d = abs(p - (min_p + max_p) * 0.5) - (max_p - min_p) * 0.5;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

float sceneDistance(vec2 position) {
    float nearest = 600.0; // realistically this is probably close enought to the largest distance we should check.

    for (int i = 0; i < BLOCKER_COUNT; ++i) {
        nearest = min(nearest, sdSquare(position, BLOCKER_BBS[i].xy, BLOCKER_BBS[i].zw));
    }

    return nearest;
}

float raymarch(vec2 position, vec2 lightPosition) {
    vec2 toLight = lightPosition - position;

    if (length(toLight) < HIT_EPSILON) {
        return 1.0;
    }

    vec2 direction = normalize(toLight);
    float t = 0.0;

    for (int i = 0; i < 32; ++i) {
        vec2 samplePosition = position + direction * t;
        if (dot(lightPosition - samplePosition, direction) < HIT_EPSILON) {
            return 1.0;
        }
        float distanceToBlocker = sceneDistance(samplePosition);
        t += distanceToBlocker;
        if (distanceToBlocker < HIT_EPSILON) {
            return 0.0;
        }
        if (t >= length(toLight)) {
            return 1.0;
        }
    }

    return 0.0; // we're probably too far away or something went wrong in our march.
}

void main() {
    vec3 light_mult = vec3(0.0);

    for (int i = 0; i < LIGHT_COUNT; ++i) {
        Light light = LIGHTS[i];

        if (!light.do_march) {
            continue;
        }

        if (length(light.position - gl_FragCoord.xy) >= light.radius) {
            continue;
        }

        float falloff = clamp(1.0 - length(light.position - gl_FragCoord.xy) / light.radius, 0.0, 1.0);
        falloff *= falloff;

        falloff = round(falloff * (light_levels - 1.0)) / (light_levels - 1.0);

        float visible = 1.0;

        if (light.do_march) {
            visible = raymarch(gl_FragCoord.xy, light.position);
        }

        light_mult += light.color * light.intensity * falloff * visible;
    }

    fragColor = vec4(clamp(light_mult + AMBIENT.rgb, 0.0, 1.0), 1.0);
}
