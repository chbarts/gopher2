typedef struct ll ll;

ll *get_next(ll * node);
ll *set_next(ll * node, ll * next);
int get_tag(ll * node);
ll *set_tag(ll * node, int tag);
void *get_data(ll * node);
ll *set_data(ll * node, void *data);
ll *get_end(ll * node);
ll *append_node(ll * cur, ll * new);
ll *prepend_node(ll * cur, ll * new);
ll *new_node(int tag, void *data, ll * next);
ll *map(ll * beg, void *(*proc) (int, void *));
ll *reverse(ll * beg);
ll *free_all_data(ll * beg);
void free_all_nodes(ll * beg);
