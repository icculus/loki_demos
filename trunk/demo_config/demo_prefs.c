
/* Load and save the preferences */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <glade/glade.h>

#define PREFS_GLADE "demo_prefs.glade"

static const char *product;
static char *title;

static struct prefentry {
    enum {
        BOOL_PREF,
        LABEL_PREF,
        FILE_PREF
    } type;

    union {
        struct {
            char *label;
            char *true_option;
            char *false_option;
            gboolean value;
        } bool_prefs;

        struct {
            char *label;
        } label_prefs;

        struct {
            char *label;
            char *format;
            char *value;
            gboolean enabled;
            GtkWidget *toggle;
            GtkWidget *entry;
            GtkWidget *button;
        } file_prefs;
    } data;

    struct prefentry *next;
} *prefs = NULL, *last_entry = NULL;


/* Parse a command line buffer into arguments */
static int parse_line(char *line, char **argv)
{
	char *bufp;
	int argc;

	argc = 0;
	for ( bufp = line; *bufp; ) {
		/* Skip leading whitespace */
		while ( isspace(*bufp) ) {
			++bufp;
		}
		/* Skip over argument */
		if ( *bufp == '"' ) {
			++bufp;
			if ( *bufp ) {
				if ( argv ) {
					argv[argc] = bufp;
				}
				++argc;
			}
			/* Skip over word */
			while ( *bufp && (*bufp != '"') ) {
				++bufp;
			}
		} else {
			if ( *bufp ) {
				if ( argv ) {
					argv[argc] = bufp;
				}
				++argc;
			}
			/* Skip over word */
			while ( *bufp && ! isspace(*bufp) ) {
				++bufp;
			}
		}
		if ( *bufp ) {
			if ( argv ) {
				*bufp = '\0';
			}
			++bufp;
		}
	}
	if ( argv ) {
		argv[argc] = NULL;
	}
	return(argc);
}

static void add_bool_option(char **args)
{
    struct prefentry *entry;

    entry = (struct prefentry *)malloc(sizeof *entry);
    if ( entry ) {
        /* Fill the data */
        entry->type = BOOL_PREF;
        entry->data.bool_prefs.label = strdup(args[1]);
        entry->data.bool_prefs.true_option = strdup(args[2]);
        entry->data.bool_prefs.false_option = strdup(args[3]);
        if ( strcasecmp(args[4], "true") == 0 ) {
            entry->data.bool_prefs.value = TRUE;
        } else {
            entry->data.bool_prefs.value = FALSE;
        }
        entry->next = NULL;

        /* Add it to our list */
        if ( last_entry ) {
            last_entry->next = entry;
        } else {
            prefs = entry;
        }
        last_entry = entry;
    }
}

static void add_label_option(char **args)
{
    struct prefentry *entry;

    entry = (struct prefentry *)malloc(sizeof *entry);
    if ( entry ) {
        /* Fill the data */
        entry->type = LABEL_PREF;
        entry->data.label_prefs.label = strdup(args[1]);
        entry->next = NULL;

        /* Add it to our list */
        if ( last_entry ) {
            last_entry->next = entry;
        } else {
            prefs = entry;
        }
        last_entry = entry;
    }
}

static void add_file_option(char **args)
{
    struct prefentry *entry;

    entry = (struct prefentry *)malloc(sizeof *entry);
    if ( entry ) {
        /* Fill the data */
        entry->type = FILE_PREF;
        entry->data.file_prefs.label = strdup(args[1]);
        entry->data.file_prefs.format = strdup(args[2]);
        entry->data.file_prefs.value = strdup(args[3]);
        if ( strcasecmp(args[4], "true") == 0 ) {
            entry->data.file_prefs.enabled = TRUE;
        } else {
            entry->data.file_prefs.enabled = FALSE;
        }
        entry->next = NULL;

        /* Add it to our list */
        if ( last_entry ) {
            last_entry->next = entry;
        } else {
            prefs = entry;
        }
        last_entry = entry;
    }
}

int load_prefs(const char *product)
{
    char path[PATH_MAX];
    FILE *fp;
    int lineno;
    char line[1024];
    int nargs;
    char **args;

    /* Open the existing preferences file */
    sprintf(path, "%s/.loki/loki_demos/%s/prefs.txt", getenv("HOME"), product);
    fp = fopen(path, "r");
    if ( ! fp ) {
        sprintf(path, "./demos/%s/launch/prefs.txt", product);
        fp = fopen(path, "r");
    }
    if ( ! fp ) {
        return(-1);
    }

    /* Load the title line */
    if ( ! fgets(line, sizeof(line), fp) ) {
        fclose(fp);
        unlink(path);
        return(-1);
    }
    line[strlen(line)-1] = '\0';
    title = strdup(line);

    /* Load the rest of the options */
    lineno = 0;
    while ( fgets(line, sizeof(line), fp) ) {
        line[strlen(line)-1] = '\0';
        ++lineno;

	    /* Parse it into arguments */
	    nargs = parse_line(line, NULL);
        if ( nargs < 1 ) {
            continue;
        }
	    args = (char **)malloc((nargs+1)*(sizeof *args));
	    if ( args == NULL ) {
            continue;
	    }
	    parse_line(line, args);

        if ( strcasecmp(args[0], "BOOL") == 0 ) {
            if ( nargs == 5 ) {
                add_bool_option(args);
            } else {
                fprintf(stderr, "Line %d: BOOL requires 4 arguments\n", lineno);
            }
        } else
        if ( strcasecmp(args[0], "LABEL") == 0 ) {
            if ( nargs == 2 ) {
                add_label_option(args);
            } else {
                fprintf(stderr, "Line %d: LABEL requires an argument\n",lineno);
            }
        } else
        if ( strcasecmp(args[0], "FILE") == 0 ) {
            if ( nargs == 5 ) {
                add_file_option(args);
            } else {
                fprintf(stderr, "Line %d: FILE requires 4 arguments\n", lineno);
            }
        } else {
            fprintf(stderr, "Line %d: Unknown keyword\n", lineno);
        }
        free(args);
    }
    fclose(fp);
    return(0);
}

int save_prefs(const char *product)
{
    char path[PATH_MAX];
    FILE *fp;
    struct prefentry *entry;
    char command[2*PATH_MAX];

    /* Save the preferences */
    sprintf(path, "%s/.loki", getenv("HOME"));
    mkdir(path, 0700);
    strcat(path, "/loki_demos");
    mkdir(path, 0700);
    strcat(path, "/");
    strcat(path, product);
    mkdir(path, 0700);
    strcat(path, "/prefs.txt");
    fp = fopen(path, "w");
    if ( ! fp ) {
        fprintf(stderr, "Unable to write to %s\n", path);
        return(-1);
    }
    fprintf(fp, "%s\n", title);
    
    for ( entry = prefs; entry; entry = entry->next ) {
        switch(entry->type) {
            case BOOL_PREF:
                fprintf(fp, "BOOL \"%s\" \"%s\" \"%s\" %s\n",
                    entry->data.bool_prefs.label,
                    entry->data.bool_prefs.true_option,
                    entry->data.bool_prefs.false_option,
                    entry->data.bool_prefs.value ? "TRUE" : "FALSE");
                break;
            case LABEL_PREF:
                fprintf(fp, "LABEL \"%s\"\n", entry->data.bool_prefs.label);
                break;
            case FILE_PREF:
                free(entry->data.file_prefs.value);
                entry->data.file_prefs.value = gtk_entry_get_text(
                        GTK_ENTRY(entry->data.file_prefs.entry));
                fprintf(fp, "FILE \"%s\" \"%s\" \"%s\" %s\n",
                    entry->data.file_prefs.label,
                    entry->data.file_prefs.format,
                    entry->data.file_prefs.value,
                    entry->data.file_prefs.enabled ? "TRUE" : "FALSE");
                break;
        }
    }
    fclose(fp);

    /* Create a command line from them */
    command[0] = '\0';
    sprintf(path, "demos/%s/launch/launch.txt", product);
    fp = fopen(path, "r");
    if ( fp ) {
        if ( fgets(command, sizeof(command), fp) ) {
            command[strlen(command)-1] = '\0';
        }
    }
    if ( command[0] ) {
        sprintf(path, "%s/.loki/loki_demos/%s/launch.txt",
                getenv("HOME"), product);
        fp = fopen(path, "w");
        if ( ! fp ) {
            fprintf(stderr, "Unable to write to %s\n", path);
            return(-1);
        }
        for ( entry = prefs; entry; entry = entry->next ) {
            switch(entry->type) {
                case BOOL_PREF:
                    strcat(command, " ");
                    if ( entry->data.bool_prefs.value ) {
                        strcat(command, entry->data.bool_prefs.true_option);
                    } else {
                        strcat(command, entry->data.bool_prefs.false_option);
                    }
                    break;
                case LABEL_PREF:
                    break;
                case FILE_PREF:
                    if ( entry->data.file_prefs.enabled ) {
                        strcat(command, " ");
                        sprintf(&command[strlen(command)],
                            entry->data.file_prefs.format,
                            entry->data.file_prefs.value);
                    }
                    break;
            }
        }
        fprintf(fp, "%s\n", command);
        fclose(fp);
    }
    return(0);
}

static GladeXML *prefs_glade;
static GladeXML *file_glade = NULL;

static void message(const char *text)
{
    GladeXML *message_glade;
    GtkWidget *window;
    GtkWidget *widget;

    /* Open the glade file and show the window */
    message_glade = glade_xml_new(PREFS_GLADE, "message"); 
    glade_xml_signal_autoconnect(message_glade);
    widget = glade_xml_get_widget(message_glade, "message_label");
    gtk_label_set_text(GTK_LABEL(widget), text);
    window = glade_xml_get_widget(message_glade, "message");
    gtk_widget_realize(window);

    /* Wait for the message window to quit */
    gtk_main();

    /* Clean up the glade file */
    gtk_object_unref(GTK_OBJECT(message_glade));
}

static void close_file_dialog(void)
{
    GtkWidget *window;

    /* Clean up the glade file */
    if ( file_glade ) {
        window = glade_xml_get_widget(file_glade, "file_dialog");
        gtk_widget_destroy(window);
        gtk_object_unref(GTK_OBJECT(file_glade));
        file_glade = NULL;
    }
}

void file_okay_slot(GtkWidget *w, gpointer data)
{
    struct prefentry *entry;
    GtkWidget *window;

    /* Find the top level window */
    window = glade_xml_get_widget(file_glade, "file_dialog");

    /* Set the text entry with the filename from the dialog */
    entry = (struct prefentry *)gtk_object_get_data(GTK_OBJECT(window), "data");
    gtk_entry_set_text(GTK_ENTRY(entry->data.file_prefs.entry),
        gtk_file_selection_get_filename(GTK_FILE_SELECTION(window)));

    close_file_dialog();
}

void file_cancel_slot(GtkWidget *w, gpointer data)
{
    close_file_dialog();
}

void file_button_slot(GtkWidget *w, gpointer data)
{
    GtkWidget *window;

    /* Open the glade file and show the window */
    if ( ! file_glade ) {
        file_glade = glade_xml_new(PREFS_GLADE, "file_dialog"); 
        glade_xml_signal_autoconnect(file_glade);
        window = glade_xml_get_widget(file_glade, "file_dialog");
        gtk_widget_realize(window);
        gtk_object_set_data(GTK_OBJECT(window), "data", data);
    }
}

void quit_ui(void)
{
    GtkWidget *window;

    /* Close the main window and clean up the glade file */
    if ( prefs_glade ) {
        window = glade_xml_get_widget(prefs_glade, "preferences");
        gtk_widget_destroy(window);
        gtk_object_unref(GTK_OBJECT(prefs_glade));
        prefs_glade = NULL;
    }
}

void cancel_button_slot(GtkWidget *w, gpointer data)
{
    gtk_main_quit();
}

void save_button_slot(GtkWidget *w, gpointer data)
{
    if ( save_prefs(product) < 0 ) {
        quit_ui();
        message("Unable to save preferences");
    } else {
        gtk_main_quit();
    }
}

int init_ui(int argc, char *argv[])
{
    GtkWidget *window;

    gtk_init(&argc,&argv);

    /* Initialize Glade */
    glade_init();
    prefs_glade = glade_xml_new(PREFS_GLADE, "preferences"); 
    if ( ! prefs_glade ) {
        return(-1);
    }

    /* Add all signal handlers defined in glade file */
    glade_xml_signal_autoconnect(prefs_glade);

    /* Show the main window */
    window = glade_xml_get_widget(prefs_glade, "preferences");
    gtk_widget_realize(window);

    return(0);
}

static void bool_toggle_option( GtkWidget* widget, gpointer func_data)
{
    struct prefentry *entry = (struct prefentry *)func_data;

    if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ) {
        entry->data.bool_prefs.value = TRUE;
    } else {
        entry->data.bool_prefs.value = FALSE;
    }
}

static void file_toggle_option( GtkWidget* widget, gpointer func_data)
{
    struct prefentry *entry = (struct prefentry *)func_data;

    if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ) {
        entry->data.file_prefs.enabled = TRUE;
    } else {
        entry->data.file_prefs.enabled = FALSE;
    }
    gtk_widget_set_sensitive(entry->data.file_prefs.entry,
                             entry->data.file_prefs.enabled);
    gtk_widget_set_sensitive(entry->data.file_prefs.button,
                             entry->data.file_prefs.enabled);
}

static void empty_container(GtkWidget *widget, gpointer data)
{
    gtk_container_remove(GTK_CONTAINER(data), widget);
}

void fill_ui(void)
{
    struct prefentry *entry;
    GtkWidget *widget;
    GtkWidget *vbox, *hbox;

    /* Set the main product text */
    widget = glade_xml_get_widget(prefs_glade, "product_label");
    gtk_label_set_text(GTK_LABEL(widget), title);

    /* Add all the option widgets */
    vbox = glade_xml_get_widget(prefs_glade, "option_vbox");
    if ( ! vbox ) {
        fprintf(stderr, "option_vbox not found in glade file!\n");
        return;
    }
    gtk_container_foreach(GTK_CONTAINER(vbox), empty_container, vbox);
    for ( entry = prefs; entry; entry = entry->next ) {
        switch(entry->type) {
            case BOOL_PREF:
                /* Create the toggle button itself */
                widget = gtk_check_button_new_with_label(
                            entry->data.bool_prefs.label);
                gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
                            entry->data.bool_prefs.value);
                gtk_signal_connect(GTK_OBJECT(widget), "toggled",
                    GTK_SIGNAL_FUNC(bool_toggle_option), (gpointer)entry);
                gtk_widget_show(widget);
                break;

            case LABEL_PREF:
                widget = gtk_label_new(entry->data.label_prefs.label);
                gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);
                gtk_misc_set_alignment(GTK_MISC(widget), 0, .5);
                gtk_widget_show(widget);
                break;

            case FILE_PREF:
                /* Create an hbox for all the new widgets */
                hbox = gtk_hbox_new(FALSE, 4);
                gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
                gtk_widget_show(hbox);
                
                /* Create the toggle button */
                widget = gtk_check_button_new_with_label(
                            entry->data.file_prefs.label);
                gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
                            entry->data.file_prefs.enabled);
                gtk_signal_connect(GTK_OBJECT(widget), "toggled",
                    GTK_SIGNAL_FUNC(file_toggle_option), (gpointer)entry);
                gtk_widget_show(widget);

                /* Create the value entry */
                widget = gtk_entry_new();
                gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
                gtk_entry_set_text(GTK_ENTRY(widget),
                                   entry->data.file_prefs.value);
                gtk_widget_set_sensitive(widget,
                                         entry->data.file_prefs.enabled);
                entry->data.file_prefs.entry = widget;
                gtk_widget_show(widget);

                /* Create the file dialog button */
                widget = gtk_button_new_with_label("File...");
                gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
                gtk_widget_set_sensitive(widget,
                                         entry->data.file_prefs.enabled);
                gtk_signal_connect(GTK_OBJECT(widget), "clicked",
                    GTK_SIGNAL_FUNC(file_button_slot), (gpointer)entry);
                entry->data.file_prefs.button = widget;
                gtk_widget_show(widget);
                break;
        }
    }
}

int main(int argc, char *argv[])
{
    if ( argc != 2 ) {
        fprintf(stderr, "Usage: %s <demo>\n", argv[0]);
        return(1);
    }
    product = argv[1];

    if ( init_ui(argc, argv) < 0 ) {
        fprintf(stderr, "Unable to initialize UI\n");
        return(-1);
    }
    if ( load_prefs(product) < 0 ) {
        quit_ui();
        message("Unable to load preferences");
        return(-1);
    }
    fill_ui();
    gtk_main();
    quit_ui();
    return(0);
}
