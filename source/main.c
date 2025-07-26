#include <stdlib.h>
#include <string.h>

// --- GBA Hardware Registers ---
#define REG_DISPCNT *(volatile unsigned short *)0x04000000
#define PALETTE_MEM ((volatile unsigned short *)0x05000000)
#define REG_VCOUNT *(volatile unsigned short *)0x04000006
#define REG_KEYINPUT *(volatile unsigned short *)0x04000130

// --- Display Constants ---
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160
#define SCREEN_X_MIN 0
#define SCREEN_X_MAX (SCREEN_WIDTH - 1)
#define SCREEN_Y_MIN 0
#define SCREEN_Y_MAX (SCREEN_HEIGHT - 1)

// Video modes and display options
#define MODE4 0x0004
#define BG2_ENABLE 0x0400
#define DISP_BACKBUFFER 0x0010

// VRAM pages for double buffering
#define VRAM_PAGE_SIZE 0xA000
#define VRAM_PAGE0 ((volatile unsigned short *)0x06000000)
#define VRAM_PAGE1 ((volatile unsigned short *)0x0600A000)
volatile unsigned short* back_buffer = VRAM_PAGE0;

// --- Input Constants ---
#define KEY_A 0x0001

// --- Math and Camera ---
#define FIXED_SHIFT 8
#define VIEWER_DISTANCE 256
#define Z_OFFSET 120

// --- Data Structures ---
typedef struct { int x, y, z; } Point3D;
typedef struct { int x, y; } Point2D;
enum ModelType { MODEL_CUBE, MODEL_TORUS };

// --- Cube Model Data ---
#define NUM_CUBE_VERTICES 8
#define NUM_CUBE_EDGES 12
Point3D cube_vertices[NUM_CUBE_VERTICES] = {
    {-30, -30, -30}, {30, -30, -30}, {30, 30, -30}, {-30, 30, -30},
    {-30, -30,  30}, {30, -30,  30}, {30, 30,  30}, {-30, 30,  30}
};
unsigned short cube_edges[NUM_CUBE_EDGES][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4},
    {0, 4}, {1, 5}, {2, 6}, {3, 7}
};

// --- Torus Model Data ---
#define NUM_MAJOR_SEGMENTS 16
#define NUM_MINOR_SEGMENTS 8
#define NUM_TORUS_VERTICES (NUM_MAJOR_SEGMENTS * NUM_MINOR_SEGMENTS)
#define NUM_TORUS_EDGES (NUM_MAJOR_SEGMENTS * NUM_MINOR_SEGMENTS * 2)
Point3D torus_vertices[NUM_TORUS_VERTICES];
unsigned short torus_edges[NUM_TORUS_EDGES][2];

// --- Sine Lookup Table ---
const short sin_lut[256] = {0, 6, 13, 19, 25, 31, 38, 44, 50, 56, 62, 68, 74, 80, 86, 92, 98, 103, 109, 115, 120, 126, 131, 136, 142, 147, 152, 157, 162, 167, 171, 176, 180, 185, 189, 193, 197, 201, 205, 209, 213, 216, 220, 223, 227, 230, 233, 236, 239, 241, 244, 246, 248, 250, 252, 254, 255, 256, 256, 256, 256, 255, 254, 252, 250, 248, 246, 244, 241, 239, 236, 233, 230, 227, 223, 220, 216, 213, 209, 205, 201, 197, 193, 189, 185, 180, 176, 171, 167, 162, 157, 152, 147, 142, 136, 131, 126, 120, 115, 109, 103, 98, 92, 86, 80, 74, 68, 62, 56, 50, 44, 38, 31, 25, 19, 13, 6, 0, -6, -13, -19, -25, -31, -38, -44, -50, -56, -62, -68, -74, -80, -86, -92, -98, -103, -109, -115, -120, -126, -131, -136, -142, -147, -152, -157, -162, -167, -171, -176, -180, -185, -189, -193, -197, -201, -205, -209, -213, -216, -220, -223, -227, -230, -233, -236, -239, -241, -244, -246, -248, -250, -252, -254, -255, -256, -256, -256, -256, -255, -254, -252, -250, -248, -246, -244, -241, 239, -236, -233, -230, -227, -223, -220, -216, -213, -209, -205, -201, -197, -193, -189, -185, -180, -176, -171, -167, -162, -157, -152, -147, -142, -136, -131, -126, -120, -115, -109, -103, -98, -92, -86, -80, -74, -68, -62, -56, -50, -44, -38, -31, -25, -19, -13, -6};

// --- Model Generation ---
void generate_torus(int major_radius, int minor_radius) {
    int vertex_index = 0;
    for (int i = 0; i < NUM_MAJOR_SEGMENTS; i++) {
        unsigned char u_angle = (i * 256) / NUM_MAJOR_SEGMENTS;
        short cos_u = sin_lut[(u_angle + 64) & 0xFF], sin_u = sin_lut[u_angle];
        for (int j = 0; j < NUM_MINOR_SEGMENTS; j++) {
            unsigned char v_angle = (j * 256) / NUM_MINOR_SEGMENTS;
            short cos_v = sin_lut[(v_angle + 64) & 0xFF], sin_v = sin_lut[v_angle];
            int R_plus_r_cos_v = major_radius + ((minor_radius * cos_v) >> FIXED_SHIFT);
            torus_vertices[vertex_index].x = (R_plus_r_cos_v * cos_u) >> FIXED_SHIFT;
            torus_vertices[vertex_index].y = (R_plus_r_cos_v * sin_u) >> FIXED_SHIFT;
            torus_vertices[vertex_index].z = (minor_radius * sin_v) >> FIXED_SHIFT;
            vertex_index++;
        }
    }
    int edge_index = 0;
    for (int i = 0; i < NUM_MAJOR_SEGMENTS; i++) {
        for (int j = 0; j < NUM_MINOR_SEGMENTS; j++) {
            int current_v = i * NUM_MINOR_SEGMENTS + j;
            int next_major_v = ((i + 1) % NUM_MAJOR_SEGMENTS) * NUM_MINOR_SEGMENTS + j;
            int next_minor_v = i * NUM_MINOR_SEGMENTS + ((j + 1) % NUM_MINOR_SEGMENTS);
            torus_edges[edge_index][0] = current_v;
            torus_edges[edge_index][1] = next_major_v;
            edge_index++;
            torus_edges[edge_index][0] = current_v;
            torus_edges[edge_index][1] = next_minor_v;
            edge_index++;
        }
    }
}

// --- Graphics Functions ---
void flip_page() { if (back_buffer == VRAM_PAGE0) { REG_DISPCNT &= ~DISP_BACKBUFFER; back_buffer = VRAM_PAGE1; } else { REG_DISPCNT |= DISP_BACKBUFFER; back_buffer = VRAM_PAGE0; } }
void vsync() { while (REG_VCOUNT >= 160); while (REG_VCOUNT < 160); }
void plot_pixel(int x, int y, unsigned char color) { if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return; int i = (y * SCREEN_WIDTH + x) >> 1; unsigned short p = back_buffer[i]; if (x & 1) p = (p & 0x00FF) | (color << 8); else p = (p & 0xFF00) | color; back_buffer[i] = p; }
void clear_screen(unsigned char color) { unsigned short v = (color << 8) | color; memset((void*)back_buffer, v, VRAM_PAGE_SIZE); }
void draw_line(int x0, int y0, int x1, int y1, unsigned char color) { int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1; int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1; int err = dx + dy, e2; for (;;) { plot_pixel(x0, y0, color); if (x0 == x1 && y0 == y1) break; e2 = 2 * err; if (e2 >= dy) { err += dy; x0 += sx; } if (e2 <= dx) { err += dx; y0 += sy; } } }

// --- Clipping ---
#define CLIP_INSIDE 0
#define CLIP_LEFT   1
#define CLIP_RIGHT  2
#define CLIP_BOTTOM 4
#define CLIP_TOP    8

int compute_outcode(int x, int y) {
    int code = CLIP_INSIDE;
    if (x < SCREEN_X_MIN) code |= CLIP_LEFT;
    else if (x > SCREEN_X_MAX) code |= CLIP_RIGHT;
    if (y < SCREEN_Y_MIN) code |= CLIP_BOTTOM;
    else if (y > SCREEN_Y_MAX) code |= CLIP_TOP;
    return code;
}

int clip_test(long p, long q, long* t0, long* t1) { long r; if (p == 0 && q < 0) return 0; if (p != 0) { r = (q << FIXED_SHIFT) / p; if (p < 0) { if (r > *t1) return 0; if (r > *t0) *t0 = r; } else { if (r < *t0) return 0; if (r < *t1) *t1 = r; } } return 1; }
int liang_barsky_clip(int* x0, int* y0, int* x1, int* y1) { long dx = *x1 - *x0, dy = *y1 - *y0; long t0 = 0, t1 = 1 << FIXED_SHIFT; if (!clip_test(-dx, *x0 - SCREEN_X_MIN, &t0, &t1)) return 0; if (!clip_test(dx, SCREEN_X_MAX - *x0, &t0, &t1)) return 0; if (!clip_test(-dy, *y0 - SCREEN_Y_MIN, &t0, &t1)) return 0; if (!clip_test(dy, SCREEN_Y_MAX - *y0, &t0, &t1)) return 0; if (t1 < (1 << FIXED_SHIFT)) { *x1 = *x0 + ((t1 * dx) >> FIXED_SHIFT); *y1 = *y0 + ((t1 * dy) >> FIXED_SHIFT); } if (t0 > 0) { *x0 = *x0 + ((t0 * dx) >> FIXED_SHIFT); *y0 = *y0 + ((t0 * dy) >> FIXED_SHIFT); } return 1; }

// --- Main Application ---
int main() {
    PALETTE_MEM[0] = 0x0000;
    PALETTE_MEM[1] = 0x7FFF;
    REG_DISPCNT = MODE4 | BG2_ENABLE;

    generate_torus(50, 20);

    enum ModelType current_model = MODEL_CUBE;
    unsigned short last_keys = 0;
    unsigned char angle_x = 0, angle_y = 0, scale_angle = 0;
    Point2D projected_points[NUM_TORUS_VERTICES];

    while (1) {
        // --- Input ---
        unsigned short current_keys = ~REG_KEYINPUT;
        if ((current_keys & KEY_A) && !(last_keys & KEY_A)) {
            current_model = (current_model == MODEL_CUBE) ? MODEL_TORUS : MODEL_CUBE;
        }
        last_keys = current_keys;

        // --- Logic ---
        clear_screen(0);
        short sin_x = sin_lut[angle_x], cos_x = sin_lut[(angle_x + 64) & 0xFF];
        short sin_y = sin_lut[angle_y], cos_y = sin_lut[(angle_y + 64) & 0xFF];
        short scale_val = 128 + (sin_lut[scale_angle] >> 1);

        Point3D* vertices;
        unsigned short (*edges)[2];
        int num_vertices, num_edges;

        if (current_model == MODEL_CUBE) {
            vertices = cube_vertices;
            edges = cube_edges;
            num_vertices = NUM_CUBE_VERTICES;
            num_edges = NUM_CUBE_EDGES;
        } else {
            vertices = torus_vertices;
            edges = torus_edges;
            num_vertices = NUM_TORUS_VERTICES;
            num_edges = NUM_TORUS_EDGES;
        }

        // --- Render ---
        for (int i = 0; i < num_vertices; i++) {
            Point3D p = vertices[i];
            p.x = (p.x * scale_val) >> FIXED_SHIFT;
            p.y = (p.y * scale_val) >> FIXED_SHIFT;
            p.z = (p.z * scale_val) >> FIXED_SHIFT;

            Point3D temp, rotated;
            temp.x = (p.x * cos_y - p.z * sin_y) >> FIXED_SHIFT;
            temp.z = (p.x * sin_y + p.z * cos_y) >> FIXED_SHIFT;
            temp.y = p.y;
            rotated.y = (temp.y * cos_x - temp.z * sin_x) >> FIXED_SHIFT;
            rotated.z = (temp.y * sin_x + temp.z * cos_x) >> FIXED_SHIFT;
            rotated.x = temp.x;
            rotated.z += Z_OFFSET;

            if (rotated.z > 0) {
                int p_factor = (VIEWER_DISTANCE << FIXED_SHIFT) / rotated.z;
                projected_points[i].x = ((rotated.x * p_factor) >> FIXED_SHIFT) + (SCREEN_WIDTH / 2);
                projected_points[i].y = ((rotated.y * p_factor) >> FIXED_SHIFT) + (SCREEN_HEIGHT / 2);
            } else {
                projected_points[i].x = -10000;
            }
        }

        for (int i = 0; i < num_edges; i++) {
            int p1_idx = edges[i][0], p2_idx = edges[i][1];
            if (projected_points[p1_idx].x == -10000 || projected_points[p2_idx].x == -10000) continue;
            
            int x0 = projected_points[p1_idx].x, y0 = projected_points[p1_idx].y;
            int x1 = projected_points[p2_idx].x, y1 = projected_points[p2_idx].y;

            int outcode0 = compute_outcode(x0, y0);
            int outcode1 = compute_outcode(x1, y1);

            if ((outcode0 | outcode1) == 0) {
                draw_line(x0, y0, x1, y1, 1); // Trivial accept
            } else if ((outcode0 & outcode1) != 0) {
                continue; // Trivial reject
            } else {
                if (liang_barsky_clip(&x0, &y0, &x1, &y1)) { // Clip
                    draw_line(x0, y0, x1, y1, 1);
                }
            }
        }

        vsync();
        flip_page();

        angle_x += 2;
        angle_y++;
        scale_angle++;
    }
    return 0;
}