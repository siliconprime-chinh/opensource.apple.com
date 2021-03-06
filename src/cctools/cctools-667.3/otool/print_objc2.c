#include "stdio.h"
#include "stddef.h"
#include "string.h"
#include "mach-o/loader.h"
#include "stuff/allocate.h"
#include "stuff/bytesex.h"
#include "stuff/symbol.h"
#include "stuff/reloc.h"
#include "ofile_print.h"

extern char *oname;

/*
 * Here we need structures that have the same memory layout and size as the
 * 64-bit Objective-C meta data structures that can be used in this 32-bit
 * program.
 *
 * The real structure definitions come from the objc4 project in the private
 * header file runtime/objc-runtime-new.h in that project.
 */

struct class_t {
    uint64_t isa;		/* class_t * (64-bit pointer) */
    uint64_t superclass;	/* class_t * (64-bit pointer) */
    uint64_t cache;		/* Cache (64-bit pointer) */
    uint64_t vtable;		/* IMP * (64-bit pointer) */
    uint64_t data;		/* class_ro_t * (64-bit pointer) */
};

static
void
swap_class_t(
struct class_t *c,
enum byte_sex target_byte_sex)
{
	c->isa = SWAP_LONG_LONG(c->isa);
	c->superclass = SWAP_LONG_LONG(c->superclass);
	c->cache = SWAP_LONG_LONG(c->cache);
	c->vtable = SWAP_LONG_LONG(c->vtable);
	c->data = SWAP_LONG_LONG(c->data);
}

struct class_ro_t {
    uint32_t flags;
    uint32_t instanceStart;
    uint32_t instanceSize;
    uint32_t reserved;
    uint64_t ivarLayout;	/* const uint8_t * (64-bit pointer) */
    uint64_t name; 		/* const char * (64-bit pointer) */
    uint64_t baseMethods; 	/* const method_list_t * (64-bit pointer) */
    uint64_t baseProtocols; 	/* const protocol_list_t * (64-bit pointer) */
    uint64_t ivars; 		/* const ivar_list_t * (64-bit pointer) */
    uint64_t weakIvarLayout; 	/* const uint8_t * (64-bit pointer) */
    uint64_t baseProperties; 	/* const struct objc_property_list *
							(64-bit pointer) */
};

/* Values for class_ro_t->flags */
#define RO_META               (1<<0)
#define RO_ROOT               (1<<1)
#define RO_HAS_CXX_STRUCTORS  (1<<2)


static
void
swap_class_ro_t(
struct class_ro_t *cro,
enum byte_sex target_byte_sex)
{
	cro->flags = SWAP_LONG(cro->flags);
	cro->instanceStart = SWAP_LONG(cro->instanceStart);
	cro->instanceSize = SWAP_LONG(cro->instanceSize);
	cro->reserved = SWAP_LONG(cro->reserved);
	cro->ivarLayout = SWAP_LONG_LONG(cro->ivarLayout);
	cro->name = SWAP_LONG_LONG(cro->name);
	cro->baseMethods = SWAP_LONG_LONG(cro->baseMethods);
	cro->baseProtocols = SWAP_LONG_LONG(cro->baseProtocols);
	cro->ivars = SWAP_LONG_LONG(cro->ivars);
	cro->weakIvarLayout = SWAP_LONG_LONG(cro->weakIvarLayout);
	cro->baseProperties = SWAP_LONG_LONG(cro->baseProperties);
}

struct method_list_t {
    uint32_t entsize;
    uint32_t count;
    /* struct method_t first;  These structures follow inline */
};

static
void
swap_method_list_t(
struct method_list_t *ml,
enum byte_sex target_byte_sex)
{
	ml->entsize = SWAP_LONG(ml->entsize);
	ml->count = SWAP_LONG(ml->count);
}

struct method_t {
    uint64_t name;	/* SEL (64-bit pointer) */
    uint64_t types;	/* const char * (64-bit pointer) */
    uint64_t imp;	/* IMP (64-bit pointer) */
};

static
void
swap_method_t(
struct method_t *m,
enum byte_sex target_byte_sex)
{
	m->name = SWAP_LONG_LONG(m->name);
	m->types = SWAP_LONG_LONG(m->types);
	m->imp = SWAP_LONG_LONG(m->imp);
}

struct ivar_list_t {
    uint32_t entsize;
    uint32_t count;
    /* struct ivar_t first;  These structures follow inline */
};

static
void
swap_ivar_list_t(
struct ivar_list_t *il,
enum byte_sex target_byte_sex)
{
	il->entsize = SWAP_LONG(il->entsize);
	il->count = SWAP_LONG(il->count);
}

struct ivar_t {
    uint64_t offset;	/* uintptr_t * (64-bit pointer) */
    uint64_t name;	/* const char * (64-bit pointer) */
    uint64_t type;	/* const char * (64-bit pointer) */
    uint32_t alignment;
    uint32_t size;
};

static
void
swap_ivar_t(
struct ivar_t *i,
enum byte_sex target_byte_sex)
{
	i->offset = SWAP_LONG_LONG(i->offset);
	i->name = SWAP_LONG_LONG(i->name);
	i->type = SWAP_LONG_LONG(i->type);
	i->alignment = SWAP_LONG(i->alignment);
	i->size = SWAP_LONG(i->size);
}

struct protocol_list_t {
    uint64_t count;	/* uintptr_t (a 64-bit value) */
    /* struct protocol_t * list[0];  These pointers follow inline */
};

static
void
swap_protocol_list_t(
struct protocol_list_t *pl,
enum byte_sex target_byte_sex)
{
	pl->count = SWAP_LONG(pl->count);
}

struct protocol_t {
    uint64_t isa;			/* id * (64-bit pointer) */
    uint64_t name;			/* const char * (64-bit pointer) */
    uint64_t protocols;			/* struct protocol_list_t *
							(64-bit pointer) */
    uint64_t instanceMethods;		/* method_list_t * (64-bit pointer) */
    uint64_t classMethods;		/* method_list_t * (64-bit pointer) */
    uint64_t optionalInstanceMethods;	/* method_list_t * (64-bit pointer) */
    uint64_t optionalClassMethods;	/* method_list_t * (64-bit pointer) */
    uint64_t instanceProperties;	/* struct objc_property_list *
							   (64-bit pointer) */
};

static
void
swap_protocol_t(
struct protocol_t *p,
enum byte_sex target_byte_sex)
{
	p->isa = SWAP_LONG_LONG(p->isa);
	p->name = SWAP_LONG_LONG(p->name);
	p->protocols = SWAP_LONG_LONG(p->protocols);
	p->instanceMethods = SWAP_LONG_LONG(p->instanceMethods);
	p->classMethods = SWAP_LONG_LONG(p->classMethods);
	p->optionalInstanceMethods = SWAP_LONG_LONG(p->optionalInstanceMethods);
	p->optionalClassMethods = SWAP_LONG_LONG(p->optionalClassMethods);
	p->instanceProperties = SWAP_LONG_LONG(p->instanceProperties);
}

struct objc_property_list {
    uint32_t entsize;
    uint32_t count;
    /* struct objc_property first;  These structures follow inline */
};

static
void
swap_objc_property_list(
struct objc_property_list *pl,
enum byte_sex target_byte_sex)
{
	pl->entsize = SWAP_LONG(pl->entsize);
	pl->count = SWAP_LONG(pl->count);
}

struct objc_property {
    uint64_t name;		/* const char * (64-bit pointer) */
    uint64_t attributes;	/* const char * (64-bit pointer) */
};

static
void
swap_objc_property(
struct objc_property *op,
enum byte_sex target_byte_sex)
{
	op->name = SWAP_LONG_LONG(op->name);
	op->attributes = SWAP_LONG_LONG(op->attributes);
}

struct category_t {
    uint64_t name; 		/* const char * (64-bit pointer) */
    uint64_t cls;		/* struct class_t * (64-bit pointer) */
    uint64_t instanceMethods;	/* struct method_list_t * (64-bit pointer) */
    uint64_t classMethods;	/* struct method_list_t * (64-bit pointer) */
    uint64_t protocols;		/* struct protocol_list_t * (64-bit pointer) */
    uint64_t instanceProperties; /* struct objc_property_list *
				    (64-bit pointer) */
} category_t;

static
void
swap_category_t(
struct category_t *c,
enum byte_sex target_byte_sex)
{
	c->name = SWAP_LONG_LONG(c->name);
	c->cls = SWAP_LONG_LONG(c->cls);
	c->instanceMethods = SWAP_LONG_LONG(c->instanceMethods);
	c->classMethods = SWAP_LONG_LONG(c->classMethods);
	c->protocols = SWAP_LONG_LONG(c->protocols);
	c->instanceProperties = SWAP_LONG_LONG(c->instanceProperties);
}

struct message_ref {
    uint64_t imp;	/* IMP (64-bit pointer) */
    uint64_t sel;	/* SEL (64-bit pointer) */
};

static
void
swap_message_ref(
struct message_ref *mr,
enum byte_sex target_byte_sex)
{
	mr->imp = SWAP_LONG_LONG(mr->imp);
	mr->sel = SWAP_LONG_LONG(mr->sel);
}

struct objc_image_info {
    uint32_t version;
    uint32_t flags;
};
/* masks for objc_image_info.flags */
#define OBJC_IMAGE_IS_REPLACEMENT (1<<0)
#define OBJC_IMAGE_SUPPORTS_GC (1<<1)


static
void
swap_objc_image_info(
struct objc_image_info *o,
enum byte_sex target_byte_sex)
{
	o->version = SWAP_LONG(o->version);
	o->flags = SWAP_LONG(o->flags);
}

struct objc_string_object_64 {
    uint64_t isa;		/* class_t * (64-bit pointer) */
    uint64_t characters;	/* char * (64-bit pointer) */
    uint32_t _length;		/* number of non-NULL characters in above */
    uint32_t _pad;		/* unused padding, compiler uses .space 4 */
};

static
void
swap_string_object_64(
struct objc_string_object_64 *string_object,
enum byte_sex target_byte_sex)
{
	string_object->isa = SWAP_LONG_LONG(string_object->isa);
	string_object->characters = SWAP_LONG_LONG(string_object->characters);
	string_object->_length = SWAP_LONG(string_object->_length);
	string_object->_pad = SWAP_LONG(string_object->_pad);
}

struct info {
    enum bool swapped;
    enum byte_sex host_byte_sex;
    struct section_info_64 *sections;
    unsigned long nsections;
    cpu_type_t cputype;
    struct nlist_64 *symbols64;
    unsigned long nsymbols;
    char *strings;
    unsigned long strings_size;
    struct symbol *sorted_symbols;
    unsigned long nsorted_symbols;
    uint64_t database;
    struct relocation_info *ext_relocs;
    unsigned long next_relocs;
    struct relocation_info *loc_relocs;
    unsigned long nloc_relocs;
    enum bool verbose;
};

struct section_info_64 {
    char segname[16];
    char sectname[16];
    char *contents;
    uint64_t addr;
    uint64_t size;
    struct relocation_info *relocs;
    unsigned long nrelocs;
};

static void walk_pointer_list(
    char *listname,
    struct section_info_64 *s,
    struct info *info,
    void (*func)(uint64_t, struct info *));

static void print_class_t(
    uint64_t p,
    struct info *info);

static void print_class_ro_t(
    uint64_t p,
    struct info *info);

static void print_layout_map(
    uint64_t p,
    struct info *info);

static void print_method_list_t(
    uint64_t p,
    struct info *info,
    char *indent);

static void print_ivar_list_t(
    uint64_t p,
    struct info *info);

static void print_protocol_list_t(
    uint64_t p,
    struct info *info);

static void print_objc_property_list(
    uint64_t p,
    struct info *info);

static void print_category_t(
    uint64_t p,
    struct info *info);

static void print_message_refs(
    struct section_info_64 *s,
    struct info *info);

static void print_image_info(
    struct section_info_64 *s,
    struct info *info);

static void get_sections_64(
    struct load_command *load_commands,
    uint32_t ncmds,
    uint32_t sizeofcmds,
    enum byte_sex object_byte_sex,
    char *object_addr,
    unsigned long object_size,
    struct section_info_64 **sections,
    unsigned long *nsections,
    uint64_t *database);

static struct section_info_64 *get_section_64(
    struct section_info_64 *sections,
    unsigned long nsections,
    char *segname,
    char *sectname);

static void get_cstring_section_64(
    struct load_command *load_commands,
    uint32_t ncmds,
    uint32_t sizeofcmds,
    enum byte_sex object_byte_sex,
    char *object_addr,
    unsigned long object_size,
    struct section_info_64 *cstring_section_ptr);

static void *get_pointer_64(
    uint64_t p,
    unsigned long *offset,
    unsigned long *left,
    struct section_info_64 **s,
    struct section_info_64 *sections,
    unsigned long nsections);

static const char *get_symbol_64(
    unsigned long sect_offset,
    uint64_t database_offset,
    unsigned long long value,
    struct relocation_info *relocs,
    unsigned long nrelocs,
    struct info *info);

/*
 * Print the objc2 meta data in 64-bit Mach-O files.
 */
void
print_objc2(
cpu_type_t cputype,
struct load_command *load_commands,
uint32_t ncmds,
uint32_t sizeofcmds,
enum byte_sex object_byte_sex,
char *object_addr,
unsigned long object_size,
struct nlist_64 *symbols64,
unsigned long nsymbols,
char *strings,
unsigned long strings_size,
struct symbol *sorted_symbols,
unsigned long nsorted_symbols,
struct relocation_info *ext_relocs,
unsigned long next_relocs,
struct relocation_info *loc_relocs,
unsigned long nloc_relocs,
enum bool verbose)
{
    struct section_info_64 *s;
    struct info info;

	info.host_byte_sex = get_host_byte_sex();
	info.swapped = info.host_byte_sex != object_byte_sex;
	info.cputype = cputype;
	info.symbols64 = symbols64;
	info.nsymbols = nsymbols;
	info.strings = strings;
	info.strings_size = strings_size;
	info.sorted_symbols = sorted_symbols;
	info.nsorted_symbols = nsorted_symbols;
	info.ext_relocs = ext_relocs;
	info.next_relocs = next_relocs;
	info.loc_relocs = loc_relocs;
	info.nloc_relocs = nloc_relocs;
	info.verbose = verbose;
	get_sections_64(load_commands, ncmds, sizeofcmds, object_byte_sex,
			object_addr, object_size, &info.sections,
			&info.nsections, &info.database);

	s = get_section_64(info.sections, info.nsections,
				"__OBJC2", "__class_list");
	if(s == NULL)
	    s = get_section_64(info.sections, info.nsections,
				"__DATA", "__objc_classlist");
	walk_pointer_list("class", s, &info, print_class_t);

	s = get_section_64(info.sections, info.nsections,
				"__OBJC2", "__class_refs");
	if(s == NULL)
	    s = get_section_64(info.sections, info.nsections,
				"__DATA", "__objc_classrefs");
	walk_pointer_list("class refs", s, &info, NULL);

	s = get_section_64(info.sections, info.nsections,
				"__OBJC2", "__super_refs");
	if(s == NULL)
	    s = get_section_64(info.sections, info.nsections,
				"__DATA", "__objc_superrefs");
	walk_pointer_list("super refs", s, &info, NULL);

	s = get_section_64(info.sections, info.nsections,
				"__OBJC2", "__category_list");
	if(s == NULL)
	    s = get_section_64(info.sections, info.nsections,
				"__DATA", "__objc_catlist");
	walk_pointer_list("category", s, &info, print_category_t);

	s = get_section_64(info.sections, info.nsections,
				"__OBJC2", "__protocol_list");
	if(s == NULL)
	    s = get_section_64(info.sections, info.nsections,
				"__DATA", "__objc_protolist");
	walk_pointer_list("protocol", s, &info, NULL);

	s = get_section_64(info.sections, info.nsections,
				"__OBJC2", "__message_refs");
	if(s == NULL)
	    s = get_section_64(info.sections, info.nsections,
				"__DATA", "__objc_msgrefs");
	print_message_refs(s, &info);

	s = get_section_64(info.sections, info.nsections,
				"__OBJC", "__image_info");
	if(s == NULL)
	    s = get_section_64(info.sections, info.nsections,
				"__DATA", "__objc_imageinfo");
	print_image_info(s, &info);
}

static
void
walk_pointer_list(
char *listname,
struct section_info_64 *s,
struct info *info,
void (*func)(uint64_t, struct info *))
{
    unsigned long i, size, left;
    uint64_t p;
    const char *name;

	if(s == NULL)
	    return;

	printf("Contents of (%s,%s) section\n", s->segname, s->sectname);
	for(i = 0; i < s->size; i += sizeof(uint64_t)){

	    memset(&p, '\0', sizeof(uint64_t));
	    left = s->size - i; 
	    size = left < sizeof(uint64_t) ?
		   left : sizeof(uint64_t);
	    memcpy(&p, s->contents + i, size);

	    if(i + sizeof(uint64_t) > s->size)
		printf("%s list pointer extends past end of (%s,%s) "
		       "section\n", listname, s->segname, s->sectname);
	    printf("%016llx ", s->addr + i);

	    if(info->swapped)
		SWAP_LONG_LONG(p);
	    printf("0x%llx", p);

	    name = get_symbol_64(i, s->addr - info->database, p,
			         s->relocs, s->nrelocs, info);
	    if(name != NULL)
		printf(" %s\n", name);
	    else
		printf("\n");
	    if(func != NULL)
		func(p, info);
	}
}

static
void
print_class_t(
uint64_t p,
struct info *info)
{
    struct class_t c;
    void *r;
    unsigned long offset, left;
    struct section_info_64 *s;
    const char *name;

	r = get_pointer_64(p, &offset, &left, &s,
			   info->sections, info->nsections);
	if(r == NULL)
	    return;
	memset(&c, '\0', sizeof(struct class_t));
	if(left < sizeof(struct class_t)){
	    memcpy(&c, r, left);
	    printf("   (class_t entends past the end of the section)\n");
	}
	else
	    memcpy(&c, r, sizeof(struct class_t));
	if(info->swapped)
	    swap_class_t(&c, info->host_byte_sex);
	printf("           isa 0x%llx", c.isa);
	name = get_symbol_64(offset + offsetof(struct class_t, isa),
			     s->addr - info->database, c.isa, s->relocs,
			     s->nrelocs, info);
	if(name != NULL)
	    printf(" %s\n", name);
	else
	    printf("\n");
	printf("    superclass 0x%llx", c.superclass);
	name = get_symbol_64(offset + offsetof(struct class_t, superclass),
			     s->addr - info->database, c.superclass, s->relocs,
			     s->nrelocs, info);
	if(name != NULL)
	    printf(" %s\n", name);
	else
	    printf("\n");
	printf("         cache 0x%llx", c.cache);
	name = get_symbol_64(offset + offsetof(struct class_t, cache),
			     s->addr - info->database, c.cache, s->relocs,
			     s->nrelocs, info);
	if(name != NULL)
	    printf(" %s\n", name);
	else
	    printf("\n");
	printf("        vtable 0x%llx", c.vtable);
	name = get_symbol_64(offset + offsetof(struct class_t, vtable),
			     s->addr - info->database, c.vtable, s->relocs,
			     s->nrelocs, info);
	if(name != NULL)
	    printf(" %s\n", name);
	else
	    printf("\n");
	printf("          data 0x%llx (struct class_ro_t *)\n", c.data);
	print_class_ro_t(c.data, info);
}

static
void
print_class_ro_t(
uint64_t p,
struct info *info)
{
    struct class_ro_t cro;
    void *r;
    unsigned long offset, left;
    struct section_info_64 *s;
    const char *name;

	r = get_pointer_64(p, &offset, &left, &s, info->sections,
			   info->nsections);
	if(r == NULL)
	    return;
	memset(&cro, '\0', sizeof(struct class_ro_t));
	if(left < sizeof(struct class_ro_t)){
	    memcpy(&cro, r, left);
	    printf("   (class_ro_t entends past the end of the section)\n");
	}
	else
	    memcpy(&cro, r, sizeof(struct class_ro_t));
	if(info->swapped)
	    swap_class_ro_t(&cro, info->host_byte_sex);
	printf("                    flags 0x%x", cro.flags);
	if(cro.flags & RO_META)
	    printf(" RO_META");
	if(cro.flags & RO_ROOT)
	    printf(" RO_ROOT");
	if(cro.flags & RO_HAS_CXX_STRUCTORS)
	    printf(" RO_HAS_CXX_STRUCTORS");
	printf("\n");
	printf("            instanceStart %u\n", cro.instanceStart);
	printf("             instanceSize %u\n", cro.instanceSize);
	printf("                 reserved 0x%x\n", cro.reserved);
	printf("               ivarLayout 0x%llx\n", cro.ivarLayout);
	print_layout_map(cro.ivarLayout, info);
	printf("                     name 0x%llx", cro.name);
	name = get_pointer_64(cro.name, NULL, &left, NULL, info->sections,
			      info->nsections);
	if(name != NULL)
	    printf(" %.*s\n", (int)left, name);
	else
	    printf("\n");
	printf("              baseMethods 0x%llx (struct method_list_t *)\n",
	       cro.baseMethods);
	if(cro.baseMethods != 0)
	    print_method_list_t(cro.baseMethods, info, "");
	printf("            baseProtocols 0x%llx\n", cro.baseProtocols);
	if(cro.baseProtocols != 0)
	    print_protocol_list_t(cro.baseProtocols, info);
	printf("                    ivars 0x%llx\n", cro.ivars);
	if(cro.ivars != 0)
	    print_ivar_list_t(cro.ivars, info);
	printf("           weakIvarLayout 0x%llx\n", cro.weakIvarLayout);
	print_layout_map(cro.weakIvarLayout, info);
	printf("           baseProperties 0x%llx\n", cro.baseProperties);
	if(cro.baseProperties != 0)
	    print_objc_property_list(cro.baseProperties, info);
}

static
void
print_layout_map(
uint64_t p,
struct info *info)
{
    unsigned long offset, left;
    struct section_info_64 *s;
    char *layout_map;

	if(p == 0)
	    return;
	layout_map = get_pointer_64(p, &offset, &left, &s, 
				    info->sections, info->nsections);
	if(layout_map != NULL){
	    printf("                layout map: ");
	    do{
		printf("0x%02x ", (*layout_map) & 0xff);
		left--;
		layout_map++;
	    }while(*layout_map != '\0' && left != 0);
	    printf("\n");
	}
}

static
void
print_method_list_t(
uint64_t p,
struct info *info,
char *indent)
{
    struct method_list_t ml;
    struct method_t m;
    void *r;
    unsigned long offset, left, i;
    struct section_info_64 *s;
    const char *name;

	r = get_pointer_64(p, &offset, &left, &s, info->sections,
			   info->nsections);
	if(r == NULL)
	    return;
	memset(&ml, '\0', sizeof(struct method_list_t));
	if(left < sizeof(struct method_list_t)){
	    memcpy(&ml, r, left);
	    printf("%s   (method_list_t entends past the end of the "
		   "section)\n", indent);
	}
	else
	    memcpy(&ml, r, sizeof(struct method_list_t));
	if(info->swapped)
	    swap_method_list_t(&ml, info->host_byte_sex);
	printf("%s\t\t   entsize %u\n", indent, ml.entsize);
	printf("%s\t\t     count %u\n", indent, ml.count);

	p += sizeof(struct method_list_t);
	offset += sizeof(struct method_list_t);
	for(i = 0; i < ml.count; i++){
	    r = get_pointer_64(p, &offset, &left, &s, info->sections,
			       info->nsections);
	    if(r == NULL)
		return;
	    memset(&m, '\0', sizeof(struct method_t));
	    if(left < sizeof(struct method_t)){
		memcpy(&ml, r, left);
		printf("%s   (method_t entends past the end of the "
		       "section)\n", indent);
	    }
	    else
		memcpy(&m, r, sizeof(struct method_t));
	    if(info->swapped)
		swap_method_t(&m, info->host_byte_sex);

	    printf("%s\t\t      name 0x%llx", indent, m.name);
	    name = get_pointer_64(m.name, NULL, &left, NULL, info->sections,
				  info->nsections);
	    if(name != NULL)
		printf(" %.*s\n", (int)left, name);
	    else
		printf("\n");
	    printf("%s\t\t     types 0x%llx", indent, m.types);
	    name = get_pointer_64(m.types, NULL, &left, NULL, info->sections,
				  info->nsections);
	    if(name != NULL)
		printf(" %.*s\n", (int)left, name);
	    else
		printf("\n");
	    printf("%s\t\t       imp 0x%llx", indent, m.imp);
	    name = get_symbol_64(offset + offsetof(struct method_t, imp),
				 s->addr - info->database, m.imp, s->relocs,
				 s->nrelocs, info);
	    if(name != NULL)
		printf(" %s\n", name);
	    else
		printf("\n");

	    p += sizeof(struct method_t);
	    offset += sizeof(struct method_t);
	}
}

static
void
print_ivar_list_t(
uint64_t p,
struct info *info)
{
    struct ivar_list_t il;
    struct ivar_t i;
    void *r;
    unsigned long offset, left, j;
    struct section_info_64 *s;
    const char *name;

	r = get_pointer_64(p, &offset, &left, &s, info->sections,
			   info->nsections);
	if(r == NULL)
	    return;
	memset(&il, '\0', sizeof(struct ivar_list_t));
	if(left < sizeof(struct ivar_list_t)){
	    memcpy(&il, r, left);
	    printf("   (ivar_list_t entends past the end of the section)\n");
	}
	else
	    memcpy(&il, r, sizeof(struct ivar_list_t));
	if(info->swapped)
	    swap_ivar_list_t(&il, info->host_byte_sex);
	printf("                    entsize %u\n", il.entsize);
	printf("                      count %u\n", il.count);

	p += sizeof(struct ivar_list_t);
	offset += sizeof(struct ivar_list_t);
	for(j = 0; j < il.count; j++){
	    r = get_pointer_64(p, &offset, &left, &s, info->sections,
			       info->nsections);
	    if(r == NULL)
		return;
	    memset(&i, '\0', sizeof(struct ivar_t));
	    if(left < sizeof(struct ivar_t)){
		memcpy(&i, r, left);
		printf("   (ivar_t entends past the end of the section)\n");
	    }
	    else
		memcpy(&i, r, sizeof(struct ivar_t));
	    if(info->swapped)
		swap_ivar_t(&i, info->host_byte_sex);

	    printf("\t\t\t   offset %llu\n", i.offset);
	    printf("\t\t\t     name 0x%llx", i.name);
	    name = get_pointer_64(i.name, NULL, &left, NULL, info->sections,
				  info->nsections);
	    if(name != NULL)
		printf(" %.*s\n", (int)left, name);
	    else
		printf("\n");
	    printf("\t\t\t     type 0x%llx", i.type);
	    name = get_pointer_64(i.type, NULL, &left, NULL, info->sections,
				  info->nsections);
	    if(name != NULL)
		printf(" %.*s\n", (int)left, name);
	    else
		printf("\n");
	    printf("\t\t\talignment %u\n", i.alignment);
	    printf("\t\t\t     size %u\n", i.size);

	    p += sizeof(struct ivar_t);
	    offset += sizeof(struct ivar_t);
	}
}

static
void
print_protocol_list_t(
uint64_t p,
struct info *info)
{
    struct protocol_list_t pl;
    uint64_t q;
    struct protocol_t pc;
    void *r;
    unsigned long offset, left, i;
    struct section_info_64 *s;
    const char *name;

	r = get_pointer_64(p, &offset, &left, &s, info->sections,
			   info->nsections);
	if(r == NULL)
	    return;
	memset(&pl, '\0', sizeof(struct protocol_list_t));
	if(left < sizeof(struct protocol_list_t)){
	    memcpy(&pl, r, left);
	    printf("   (protocol_list_t entends past the end of the "
		   "section)\n");
	}
	else
	    memcpy(&pl, r, sizeof(struct protocol_list_t));
	if(info->swapped)
	    swap_protocol_list_t(&pl, info->host_byte_sex);
	printf("                      count %llu\n", pl.count);

	p += sizeof(struct protocol_list_t);
	offset += sizeof(struct protocol_list_t);
	for(i = 0; i < pl.count; i++){
	    r = get_pointer_64(p, &offset, &left, &s, info->sections,
			       info->nsections);
	    if(r == NULL)
		return;
	    q = 0;
	    if(left < sizeof(uint64_t)){
		memcpy(&q, r, left);
		printf("   (protocol_t * entends past the end of the "
		       "section)\n");
	    }
	    else
		memcpy(&q, r, sizeof(uint64_t));
	    if(info->swapped)
		q = SWAP_LONG_LONG(q);
	    printf("\t\t      list[%lu] 0x%llx (struct protocol_t *)\n", i, q);

	    r = get_pointer_64(q, &offset, &left, &s, info->sections,
			       info->nsections);
	    if(r == NULL)
		return;
	    memset(&pc, '\0', sizeof(struct protocol_t));
	    if(left < sizeof(struct protocol_t)){
		memcpy(&pc, r, left);
		printf("   (protocol_t entends past the end of the section)\n");
	    }
	    else
		memcpy(&pc, r, sizeof(struct protocol_t));
	    if(info->swapped)
		swap_protocol_t(&pc, info->host_byte_sex);

	    printf("\t\t\t      isa 0x%llx\n", pc.isa);
	    printf("\t\t\t     name 0x%llx", pc.name);
	    name = get_pointer_64(pc.name, NULL, &left, NULL, info->sections,
				  info->nsections);
	    if(name != NULL)
		printf(" %.*s\n", (int)left, name);
	    else
		printf("\n");
	    printf("\t\t\tprotocols 0x%llx\n", pc.protocols);
	    printf("\t\t  instanceMethods 0x%llx (struct method_list_t *)\n",
		   pc.instanceMethods);
	    if(pc.instanceMethods != 0)
		print_method_list_t(pc.instanceMethods, info, "\t");
	    printf("\t\t     classMethods 0x%llx (struct method_list_t *)\n",
		   pc.classMethods);
	    if(pc.classMethods != 0)
		print_method_list_t(pc.classMethods, info, "\t");
	    printf("\t  optionalInstanceMethods 0x%llx\n",
		   pc.optionalInstanceMethods);
	    printf("\t     optionalClassMethods 0x%llx\n",
		   pc.optionalClassMethods);
	    printf("\t       instanceProperties 0x%llx\n",
		   pc.instanceProperties);

	    p += sizeof(uint64_t);
	    offset += sizeof(uint64_t);
	}
}

static
void
print_objc_property_list(
uint64_t p,
struct info *info)
{
    struct objc_property_list opl;
    struct objc_property op;
    void *r;
    unsigned long offset, left, j;
    struct section_info_64 *s;
    const char *name;

	r = get_pointer_64(p, &offset, &left, &s, info->sections,
			   info->nsections);
	if(r == NULL)
	    return;
	memset(&opl, '\0', sizeof(struct objc_property_list));
	if(left < sizeof(struct objc_property_list)){
	    memcpy(&opl, r, left);
	    printf("   (objc_property_list entends past the end of the "
		   "section)\n");
	}
	else
	    memcpy(&opl, r, sizeof(struct objc_property_list));
	if(info->swapped)
	    swap_objc_property_list(&opl, info->host_byte_sex);
	printf("                    entsize %u\n", opl.entsize);
	printf("                      count %u\n", opl.count);

	p += sizeof(struct objc_property_list);
	offset += sizeof(struct objc_property_list);
	for(j = 0; j < opl.count; j++){
	    r = get_pointer_64(p, &offset, &left, &s, info->sections,
			       info->nsections);
	    if(r == NULL)
		return;
	    memset(&op, '\0', sizeof(struct objc_property));
	    if(left < sizeof(struct objc_property)){
		memcpy(&op, r, left);
		printf("   (objc_property entends past the end of the "
		       "section)\n");
	    }
	    else
		memcpy(&op, r, sizeof(struct objc_property));
	    if(info->swapped)
		swap_objc_property(&op, info->host_byte_sex);

	    printf("\t\t\t     name 0x%llx", op.name);
	    name = get_pointer_64(op.name, NULL, &left, NULL, info->sections,
				  info->nsections);
	    if(name != NULL)
		printf(" %.*s\n", (int)left, name);
	    else
		printf("\n");
	    printf("\t\t\tattributes x%llx", op.attributes);
	    name = get_pointer_64(op.attributes, NULL, &left, NULL,
				  info->sections, info->nsections);
	    if(name != NULL)
		printf(" %.*s\n", (int)left, name);
	    else
		printf("\n");

	    p += sizeof(struct objc_property);
	    offset += sizeof(struct objc_property);
	}
}

static
void
print_category_t(
uint64_t p,
struct info *info)
{
    struct category_t c;
    void *r;
    unsigned long offset, left;
    struct section_info_64 *s;
    const char *name;

	r = get_pointer_64(p, &offset, &left, &s,
			   info->sections, info->nsections);
	if(r == NULL)
	    return;
	memset(&c, '\0', sizeof(struct category_t));
	if(left < sizeof(struct category_t)){
	    memcpy(&c, r, left);
	    printf("   (category_t entends past the end of the section)\n");
	}
	else
	    memcpy(&c, r, sizeof(struct category_t));
	if(info->swapped)
	    swap_category_t(&c, info->host_byte_sex);
	printf("              name 0x%llx", c.name);
	name = get_symbol_64(offset + offsetof(struct category_t, name),
			     s->addr - info->database, c.name, s->relocs,
			     s->nrelocs, info);
	if(name != NULL)
	    printf(" %s\n", name);
	else
	    printf("\n");
	printf("               cls 0x%llx\n", c.cls);
	if(c.cls != 0)
	    print_class_t(c.cls, info);
	printf("   instanceMethods 0x%llx\n", c.instanceMethods);
	if(c.instanceMethods != 0)
	    print_method_list_t(c.instanceMethods, info, "");
	printf("      classMethods 0x%llx\n", c.classMethods);
	if(c.classMethods != 0)
	    print_method_list_t(c.classMethods, info, "");
	printf("         protocols 0x%llx\n", c.protocols);
	if(c.protocols != 0)
	    print_protocol_list_t(c.protocols, info);
	printf("instanceProperties 0x%llx\n", c.instanceProperties);
	if(c.instanceProperties)
	    print_objc_property_list(c.instanceProperties, info);
}

static
void
print_message_refs(
struct section_info_64 *s,
struct info *info)
{
    unsigned long i, left, offset;
    uint64_t p;
    struct message_ref mr;
    const char *name;
    void *r;

	if(s == NULL)
	    return;

	printf("Contents of (%s,%s) section\n", s->segname, s->sectname);
	offset = 0;
	for(i = 0; i < s->size; i += sizeof(struct message_ref)){
	    p = s->addr + i;
	    r = get_pointer_64(p, &offset, &left, &s,
			       info->sections, info->nsections);
	    if(r == NULL)
		return;
	    memset(&mr, '\0', sizeof(struct message_ref));
	    if(left < sizeof(struct message_ref)){
		memcpy(&mr, r, left);
		printf(" (message_ref entends past the end of the section)\n");
	    }
	    else
		memcpy(&mr, r, sizeof(struct message_ref));
	    if(info->swapped)
		swap_message_ref(&mr, info->host_byte_sex);
	    printf("  imp 0x%llx", mr.imp);
	    name = get_symbol_64(offset + offsetof(struct message_ref, imp),
				 s->addr - info->database, mr.imp, s->relocs,
				 s->nrelocs, info);
	    if(name != NULL)
		printf(" %s\n", name);
	    else
		printf("\n");
	    printf("  sel 0x%llx", mr.sel);
	    name = get_pointer_64(mr.sel, NULL, &left, NULL, info->sections,
				  info->nsections);
	    if(name != NULL)
		printf(" %.*s\n", (int)left, name);
	    else
		printf("\n");
	    offset += sizeof(struct message_ref);
	}
}

static
void
print_image_info(
struct section_info_64 *s,
struct info *info)
{
    unsigned long left, offset;
    uint64_t p;
    struct objc_image_info o;
    void *r;

	if(s == NULL)
	    return;

	printf("Contents of (%s,%s) section\n", s->segname, s->sectname);
	p = s->addr;
	r = get_pointer_64(p, &offset, &left, &s,
			   info->sections, info->nsections);
	if(r == NULL)
	    return;
	memset(&o, '\0', sizeof(struct objc_image_info));
	if(left < sizeof(struct objc_image_info)){
	    memcpy(&o, r, left);
	    printf(" (objc_image_info entends past the end of the section)\n");
	}
	else
	    memcpy(&o, r, sizeof(struct objc_image_info));
	if(info->swapped)
	    swap_objc_image_info(&o, info->host_byte_sex);
	printf("  version %u\n", o.version);
	printf("    flags 0x%x", o.flags);
	if(o.flags & OBJC_IMAGE_IS_REPLACEMENT)
	    printf(" OBJC_IMAGE_IS_REPLACEMENT");
	if(o.flags & OBJC_IMAGE_SUPPORTS_GC)
	    printf(" OBJC_IMAGE_SUPPORTS_GC");
	printf("\n");
}

void
print_objc_string_object_section_64(
char *sectname,
struct load_command *load_commands,
uint32_t ncmds,
uint32_t sizeofcmds,
enum byte_sex object_byte_sex,
char *object_addr,
unsigned long object_size,
cpu_type_t cputype,
struct nlist_64 *symbols64,
unsigned long nsymbols,
char *strings,
const unsigned long strings_size,
struct symbol *sorted_symbols,
unsigned long nsorted_symbols,
enum bool verbose)
{
    struct info info;
    struct section_info_64 *o, cstring_section;
    struct objc_string_object_64 *string_objects, *s, string_object;
    uint64_t string_objects_addr, string_objects_size;
    unsigned long size, left;
    char *p;
    const char *name;

	printf("Contents of (" SEG_OBJC ",%s) section\n", sectname);
	info.host_byte_sex = get_host_byte_sex();
	info.swapped = info.host_byte_sex != object_byte_sex;
	info.cputype = cputype;
	info.symbols64 = symbols64;
	info.nsymbols = nsymbols;
	info.strings = strings;
	info.strings_size = strings_size;
	info.sorted_symbols = sorted_symbols;
	info.nsorted_symbols = nsorted_symbols;
	info.verbose = verbose;
	get_sections_64(load_commands, ncmds, sizeofcmds, object_byte_sex,
			object_addr, object_size, &info.sections,
			&info.nsections, &info.database);
	o = get_section_64(info.sections, info.nsections, SEG_OBJC, sectname);
	get_cstring_section_64(load_commands, ncmds, sizeofcmds,object_byte_sex,
			       object_addr, object_size, &cstring_section);

	string_objects = (struct objc_string_object_64 *)o->contents;
	string_objects_addr = o->addr;
	string_objects_size = o->size;
	for(s = string_objects;
	    (char *)s < (char *)string_objects + string_objects_size;
	    s++){

	    memset(&string_object, '\0', sizeof(struct objc_string_object_64));
	    left = string_objects_size - (s - string_objects); 
	    size = left < sizeof(struct objc_string_object_64) ?
		   left : sizeof(struct objc_string_object_64);
	    memcpy(&string_object, s, size);

	    if((char *)s + sizeof(struct objc_string_object_64) >
	       (char *)s + string_objects_size)
		printf("String Object extends past end of %s section\n",
		       sectname);
	    printf("String Object 0x%llx\n",
		   string_objects_addr + ((char *)s - (char *)string_objects));

	    if(info.swapped)
		swap_string_object_64(&string_object, info.host_byte_sex);
	    printf("           isa 0x%llx", string_object.isa);
	    name = get_symbol_64((uintptr_t)s - (uintptr_t)string_objects,
				 o->addr - info.database, string_object.isa,
				 o->relocs, o->nrelocs, &info);
	    if(name != NULL)
		printf(" %s\n", name);
	    else
		printf("\n");
	    printf("    characters 0x%llx", string_object.characters);
	    if(verbose){
		p = get_pointer_64(string_object.characters, NULL, &left,
				   NULL, info.sections, info.nsections);
		if(p != NULL)
		    printf(" %.*s\n", (int)left, p);
	    }
	    else
		printf("\n");
	    printf("       _length %u\n", string_object._length);
	    printf("          _pad %u\n", string_object._pad);
	}
}

static
void
get_sections_64(
struct load_command *load_commands,
uint32_t ncmds,
uint32_t sizeofcmds,
enum byte_sex object_byte_sex,
char *object_addr,
unsigned long object_size,
struct section_info_64 **sections,
unsigned long *nsections,
uint64_t *database) 
{
    enum byte_sex host_byte_sex;
    enum bool swapped, database_set, zerobased;

    unsigned long i, j, left, size;
    struct load_command lcmd, *lc;
    char *p;
    struct segment_command_64 sg64;
    struct section_64 s64;

	host_byte_sex = get_host_byte_sex();
	swapped = host_byte_sex != object_byte_sex;

	*sections = NULL;
	*nsections = 0;
	database_set = FALSE;
	*database = 0;
	zerobased = FALSE;

	lc = load_commands;
	for(i = 0 ; i < ncmds; i++){
	    memcpy((char *)&lcmd, (char *)lc, sizeof(struct load_command));
	    if(swapped)
		swap_load_command(&lcmd, host_byte_sex);
	    if(lcmd.cmdsize % sizeof(long) != 0)
		printf("load command %lu size not a multiple of "
		       "sizeof(long)\n", i);
	    if((char *)lc + lcmd.cmdsize >
	       (char *)load_commands + sizeofcmds)
		printf("load command %lu extends past end of load "
		       "commands\n", i);
	    left = sizeofcmds - ((char *)lc - (char *)load_commands);

	    switch(lcmd.cmd){
	    case LC_SEGMENT_64:
		memset((char *)&sg64, '\0', sizeof(struct segment_command_64));
		size = left < sizeof(struct segment_command_64) ?
		       left : sizeof(struct segment_command_64);
		memcpy((char *)&sg64, (char *)lc, size);
		if(swapped)
		    swap_segment_command_64(&sg64, host_byte_sex);
		if((sg64.initprot & VM_PROT_WRITE) == VM_PROT_WRITE &&
		   database_set == FALSE){
		    *database = sg64.vmaddr;
		    database_set = TRUE;
		}
		if((sg64.initprot & VM_PROT_READ) == VM_PROT_READ &&
		   sg64.vmaddr == 0)
		    zerobased = TRUE;
		p = (char *)lc + sizeof(struct segment_command_64);
		for(j = 0 ; j < sg64.nsects ; j++){
		    if(p + sizeof(struct section_64) >
		       (char *)load_commands + sizeofcmds){
			printf("section structure command extends past "
			       "end of load commands\n");
		    }
		    left = sizeofcmds - (p - (char *)load_commands);
		    memset((char *)&s64, '\0', sizeof(struct section_64));
		    size = left < sizeof(struct section_64) ?
			   left : sizeof(struct section_64);
		    memcpy((char *)&s64, p, size);
		    if(swapped)
			swap_section_64(&s64, 1, host_byte_sex);

		    *sections = reallocate(*sections,
		       sizeof(struct section_info_64) * (*nsections + 1));
		    memcpy((*sections)[*nsections].segname,
			   s64.segname, 16);
		    memcpy((*sections)[*nsections].sectname,
			   s64.sectname, 16);
		    (*sections)[*nsections].addr = s64.addr;
		    (*sections)[*nsections].contents = object_addr + s64.offset;
		    if(s64.offset > object_size){
			printf("section contents of: (%.16s,%.16s) is past "
			       "end of file\n", s64.segname, s64.sectname);
			(*sections)[*nsections].size =  0;
		    }
		    else if(s64.offset + s64.size > object_size){
			printf("part of section contents of: (%.16s,%.16s) "
			       "is past end of file\n",
			       s64.segname, s64.sectname);
			(*sections)[*nsections].size = object_size - s64.offset;
		    }
		    else
			(*sections)[*nsections].size = s64.size;
		    if(s64.reloff >= object_size){
			printf("relocation entries offset for (%.16s,%.16s)"
			       ": is past end of file\n", s64.segname,
			       s64.sectname);
			(*sections)[*nsections].nrelocs = 0;
		    }
		    else{
			(*sections)[*nsections].relocs =
			    (struct relocation_info *)(object_addr +
						       s64.reloff);
			if(s64.reloff +
			   s64.nreloc * sizeof(struct relocation_info) >
							    object_size){
			    printf("relocation entries for section (%.16s,"
				   "%.16s) extends past end of file\n",
				   s64.segname, s64.sectname);
			    (*sections)[*nsections].nrelocs =
				(object_size - s64.reloff) /
					    sizeof(struct relocation_info);
			}
			else
			    (*sections)[*nsections].nrelocs = s64.nreloc;
			if(swapped)
			    swap_relocation_info(
				(*sections)[*nsections].relocs,
				(*sections)[*nsections].nrelocs,
				host_byte_sex);
		    }
		    (*nsections)++;

		    if(p + sizeof(struct section_64) >
		       (char *)load_commands + sizeofcmds)
			break;
		    p += size;
		}
		break;
	    }
	    if(lcmd.cmdsize == 0){
		printf("load command %lu size zero (can't advance to other "
		       "load commands)\n", i);
		break;
	    }
	    lc = (struct load_command *)((char *)lc + lcmd.cmdsize);
	    if((char *)lc > (char *)load_commands + sizeofcmds)
		break;
	}
	if(zerobased == TRUE)
	    *database = 0;
}

static
struct section_info_64 *
get_section_64(
struct section_info_64 *sections,
unsigned long nsections,
char *segname,
char *sectname)
{
    unsigned long i;

	for(i = 0; i < nsections; i++){
	    if(strncmp(sections[i].segname, segname, 16) == 0 &&
	       strncmp(sections[i].sectname, sectname, 16) == 0){
		return(sections + i);
	    }
	}
	return(NULL);
}

static
void
get_cstring_section_64(
struct load_command *load_commands,
uint32_t ncmds,
uint32_t sizeofcmds,
enum byte_sex object_byte_sex,
char *object_addr,
unsigned long object_size,
struct section_info_64 *cstring_section)
{
    enum byte_sex host_byte_sex;
    enum bool swapped;

    unsigned long i, j, left, size;
    struct load_command lcmd, *lc;
    char *p;
    struct segment_command_64 sg64;
    struct section_64 s64;

	host_byte_sex = get_host_byte_sex();
	swapped = host_byte_sex != object_byte_sex;

	memset(cstring_section, '\0', sizeof(struct section_info_64));

	lc = load_commands;
	for(i = 0 ; i < ncmds; i++){
	    memcpy((char *)&lcmd, (char *)lc, sizeof(struct load_command));
	    if(swapped)
		swap_load_command(&lcmd, host_byte_sex);
	    if(lcmd.cmdsize % sizeof(long) != 0)
		printf("load command %lu size not a multiple of "
		       "sizeof(long)\n", i);
	    if((char *)lc + lcmd.cmdsize >
	       (char *)load_commands + sizeofcmds)
		printf("load command %lu extends past end of load "
		       "commands\n", i);
	    left = sizeofcmds - ((char *)lc - (char *)load_commands);

	    switch(lcmd.cmd){
	    case LC_SEGMENT_64:
		memset((char *)&sg64, '\0', sizeof(struct segment_command_64));
		size = left < sizeof(struct segment_command_64) ?
		       left : sizeof(struct segment_command_64);
		memcpy((char *)&sg64, (char *)lc, size);
		if(swapped)
		    swap_segment_command_64(&sg64, host_byte_sex);

		p = (char *)lc + sizeof(struct segment_command_64);
		for(j = 0 ; j < sg64.nsects ; j++){
		    if(p + sizeof(struct section_64) >
		       (char *)load_commands + sizeofcmds){
			printf("section structure command extends past "
			       "end of load commands\n");
		    }
		    left = sizeofcmds - (p - (char *)load_commands);
		    memset((char *)&s64, '\0', sizeof(struct section_64));
		    size = left < sizeof(struct section_64) ?
			   left : sizeof(struct section_64);
		    memcpy((char *)&s64, p, size);
		    if(swapped)
			swap_section_64(&s64, 1, host_byte_sex);

		    if(strcmp(s64.segname, SEG_TEXT) == 0 &&
		       strcmp(s64.sectname, "__cstring") == 0){
			cstring_section->addr = s64.addr;
			cstring_section->contents = object_addr + s64.offset;
			if(s64.offset > object_size){
			    printf("section contents of: (%.16s,%.16s) is past "
				   "end of file\n", s64.segname, s64.sectname);
			    cstring_section->size = 0;
			}
			else if(s64.offset + s64.size > object_size){
			    printf("part of section contents of: (%.16s,%.16s) "
				   "is past end of file\n",
				   s64.segname, s64.sectname);
			    cstring_section->size = object_size - s64.offset;
			}
			else
			    cstring_section->size = s64.size;
			return;
		    }

		    if(p + sizeof(struct section) >
		       (char *)load_commands + sizeofcmds)
			break;
		    p += size;
		}
		break;
	    }
	    if(lcmd.cmdsize == 0){
		printf("load command %lu size zero (can't advance to other "
		       "load commands)\n", i);
		break;
	    }
	    lc = (struct load_command *)((char *)lc + lcmd.cmdsize);
	    if((char *)lc > (char *)load_commands + sizeofcmds)
		break;
	}
}

static
void *
get_pointer_64(
uint64_t p,
unsigned long *offset,
unsigned long *left,
struct section_info_64 **s,
struct section_info_64 *sections,
unsigned long nsections)
{
    void *r;
    uint64_t addr;
    unsigned long i;

	addr = p;
	for(i = 0; i < nsections; i++){
	    if(addr >= sections[i].addr &&
	       addr < sections[i].addr + sections[i].size){
		if(s != NULL)
		    *s = sections + i;
		if(offset != NULL)
		    *offset = addr - sections[i].addr;
		if(left != NULL)
		    *left = sections[i].size - (addr - sections[i].addr);
		r = sections[i].contents + (addr - sections[i].addr);
		return(r);
	    }
	}
	if(s != NULL)
	    *s = NULL;
	if(offset != NULL)
	    *offset = 0;
	if(left != NULL)
	    *left = 0;
	return(NULL);
}

/*
 * get_symbol() returns the name of a symbol (or NULL). Based on the relocation
 * information at the specified section offset or the value.
 */
static
const char *
get_symbol_64(
unsigned long sect_offset,
uint64_t database_offset,
unsigned long long value,
struct relocation_info *relocs,
unsigned long nrelocs,
struct info *info)
{
    unsigned long i;
    unsigned int r_symbolnum;
    unsigned long n_strx;

	if(info->verbose == FALSE)
	    return(NULL);

	for(i = 0; i < nrelocs; i++){
	    if((unsigned long)relocs[i].r_address == sect_offset){
		r_symbolnum = relocs[i].r_symbolnum;
		if(relocs[i].r_extern){
		    if(r_symbolnum >= info->nsymbols)
			break;
		    n_strx = info->symbols64[r_symbolnum].n_un.n_strx;
		    if(n_strx <= 0 || n_strx >= info->strings_size)
			break;
		    return(info->strings + n_strx);
		}
		break;
	    }
	    if(reloc_has_pair(info->cputype, relocs[i].r_type) == TRUE)
		i++;
	}
	for(i = 0; i < info->next_relocs; i++){
	    if((unsigned long)info->ext_relocs[i].r_address ==
		database_offset + sect_offset){
		r_symbolnum = info->ext_relocs[i].r_symbolnum;
		if(info->ext_relocs[i].r_extern){
		    if(r_symbolnum >= info->nsymbols)
			break;
		    n_strx = info->symbols64[r_symbolnum].n_un.n_strx;
		    if(n_strx <= 0 || n_strx >= info->strings_size)
			break;
		    return(info->strings + n_strx);
		}
		break;
	    }
	    if(reloc_has_pair(info->cputype, info->ext_relocs[i].r_type) ==TRUE)
		i++;
	}
	return(guess_symbol(value, info->sorted_symbols, info->nsorted_symbols,
			    info->verbose));
}
