
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

enum Axis2
{
  Axis2_X,
  Axis2_Y,
  Axis2_COUNT
};

typedef uint32_t UI_NodeFlags;

enum UI_NodeFlag
{
  UI_NodeFlag_Clickable       = (1<<0),
  UI_NodeFlag_ViewScroll      = (1<<1),
  UI_NodeFlag_DrawText        = (1<<2),
  UI_NodeFlag_DrawBorder      = (1<<3),
  UI_NodeFlag_DrawBackground  = (1<<4),
  UI_NodeFlag_DrawDropShadow  = (1<<5),
  UI_NodeFlag_Clip            = (1<<6),
  UI_NodeFlag_HotAnimation    = (1<<7),
  UI_NodeFlag_ActiveAnimation = (1<<8),
  // ...
};

typedef struct
{
    uint32_t hash;
} UI_Key;

typedef struct UI_Node UI_Node;
struct UI_Node
{
    // tree links
    UI_Node *first;
    UI_Node *last;
    UI_Node *next;
    UI_Node *prev;
    UI_Node *parent;

    // hash links
    UI_Node *hash_next;
    UI_Node *hash_prev;

    // key+generation info
    UI_Key key;
    uint64_t last_frame_touched_index;

    // per-frame info provided by builders
    UI_NodeFlags flags;
    char* string;
    UI_Size semantic_size[Axis2_COUNT];

    // computed every frame
    float computed_rel_position[Axis2_COUNT];
    float computed_size[Axis2_COUNT];
    Rect rect;

    // persistent data
    float hot_t;
    float active_t;

};

