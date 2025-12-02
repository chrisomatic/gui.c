
/*

    UI_Box *parent = ...;
    UI_PushParent(parent);
        if(UI_Button("Foo").clicked)
        {
            // clicked
        }
    UI_PopParent();

    1. Build UI_Box hierarchy (use last frame's data)
    2. Autolayout pass
    3. Render

    Autolayout:

    for each axis {
        // calculate standalone sizes (sizekind_pixels, sizekind_textcontent)
        // (pre-order) calculate upwards dependent sizes (percentofparent)
        // (post-order) calculate downwards dependent sizes (childrensum)
        // (pre-order) solve violations.
        // (pre-order) compute the relative positions of each box
    }

*/

typedef enum
{
  UI_SizeKind_Null,
  UI_SizeKind_Pixels,
  UI_SizeKind_TextContent,
  UI_SizeKind_PercentOfParent,
  UI_SizeKind_ChildrenSum,
} UI_SizeKind;

typedef struct
{
  UI_SizeKind kind;
  float value;
  float strictness;
} UI_Size;

typedef enum
{
  Axis2_X,
  Axis2_Y,
  Axis2_COUNT
} Axis2;

typedef uint32_t UI_BoxFlags;

enum UI_BoxFlag
{
  UI_BoxFlag_Clickable       = (1<<0),
  UI_BoxFlag_ViewScroll      = (1<<1),
  UI_BoxFlag_DrawText        = (1<<2),
  UI_BoxFlag_DrawBorder      = (1<<3),
  UI_BoxFlag_DrawBackground  = (1<<4),
  UI_BoxFlag_DrawDropShadow  = (1<<5),
  UI_BoxFlag_Clip            = (1<<6),
  UI_BoxFlag_HotAnimation    = (1<<7),
  UI_BoxFlag_ActiveAnimation = (1<<8),
  // ...
};

typedef struct
{
    uint32_t hash;
} UI_Key;

typedef struct UI_Box UI_Box;
struct UI_Box
{
    // tree links
    UI_Box *first;
    UI_Box *last;
    UI_Box *next;
    UI_Box *prev;
    UI_Box *parent;

    // hash links
    UI_Box *hash_next;
    UI_Box *hash_prev;

    // key+generation info
    UI_Key key;
    uint64_t last_frame_touched_index;

    // per-frame info provided by builders
    UI_BoxFlags flags;
    String string;
    UI_Size semantic_size[Axis2_COUNT];

    // computed every frame
    float computed_rel_position[Axis2_COUNT];
    float computed_size[Axis2_COUNT];
    Rect rect;

    // persistent data
    float hot_t;
    float active_t;
};

typedef struct UI_Comm UI_Comm;
struct UI_Comm
{
    UI_Box *box;
    Vec2f mouse;
    Vec2f drag_delta;
    B8 clicked;
    B8 double_clocked;
    B8 right_clicked;
    B8 pressed;
    B8 released;
    B8 dragging;
    B8 hovering;
};

// Basic Key-type helpers

UI_Key UI_KeyNull(void);
UI_Key UI_KeyFromString(String string);
B32 UI_KeyMatch(UI_Key a, UI_Key b);

// Construct a box, looking from the cache if possible,
// and pushing it as a new child of the active parent

UI_Box *UI_BoxMake(UI_BoxFlags flags, String str)
{
    UI_Box box;

    box.flags = flags;
    box.string = str;

    UI_Size semantic_size[Axis2_COUNT];
 
}

UI_Box *UI_BoxMakeF(UI_BoxFlags flags, char *fmt, ...);

// Other
void UI_BoxEquipDisplayString(UI_Box *box, String string);
void UI_BoxEquipChildLayoutAxis(UI_Box *box, Axis2 axis);

// Managing parent stack
UI_Box *UI_PushParent(UI_Box *box);
UI_Box *UI_PopParent(void);

// Get user communication from box
UI_Comm UI_CommFromBox(UI_Box *box)
{
    UI_Comm comm = {0};
    return comm;
};

// Widgets

UI_Comm UI_Button(String string)
{
    UI_Box *box = UI_BoxMake(
            UI_BoxFlag_Clickable |
            UI_BoxFlag_DrawText |
            UI_BoxFlag_DrawBackground |
            UI_BoxFlag_HotAnimation |
            UI_BoxFlag_ActiveAnimation,
            string);
    return UI_CommFromBox(box);
}
