
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

#define PREFS_GLADE "demo_config.glade"

static const char *product;
static char *title;

static struct prefentry {
    enum {
        BOOL_PREF,
        SEPARATOR_PREF,
        LABEL_PREF,
        RADIO_PREF,
        FILE_PREF
    } type;

    union {
        struct {
            char *label;
        } label_prefs;

        struct {
            char *label;
            char *true_option;
            char *false_option;
            gboolean value;
        } bool_prefs;

        struct {
            char *label;
            gboolean enabled;
            struct radio_option {
                char *label;
                char *value;
                gboolean enabled;
                GtkWidget *button;
                struct radio_option *next;
            } *options, *tail_option;
        } radio_prefs;

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

static void add_separator_option(char **args)
{
    struct prefentry *entry;

    entry = (struct prefentry *)malloc(sizeof *entry);
    if ( entry ) {
        /* Fill the data */
        entry->type = SEPARATOR_PREF;
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

static void add_radio_option(char **args)
{
    struct prefentry *entry;
    int i;

    entry = (struct prefentry *)malloc(sizeof *entry);
    if ( entry ) {
        /* Fill the data */
        entry->type = RADIO_PREF;
        entry->data.radio_prefs.label = strdup(args[1]);
        if ( strcasecmp(args[2], "always") == 0 ) {
            entry->data.radio_prefs.enabled = TRUE*2;
        } else
        if ( strcasecmp(args[2], "true") == 0 ) {
            entry->data.radio_prefs.enabled = TRUE;
        } else {
            entry->data.radio_prefs.enabled = FALSE;
        }
        entry->data.radio_prefs.options = NULL;
        entry->data.radio_prefs.tail_option = NULL;
        for ( i=3; args[i] && args[i+1] && args[i+2] && args[i+3]; i += 4 ) {
            struct radio_option *option;

            if ( strcasecmp(args[i], "OPTION") != 0 ) {
                /* Urkh, syntax error.. how to report it? */
                continue;
            }
            option = (struct radio_option *)malloc(sizeof *option);
            if ( option ) {
                option->label = strdup(args[i+1]);
                option->value = strdup(args[i+2]);
                if ( strcasecmp(args[i+3], "true") == 0 ) {
                    option->enabled = TRUE;
                } else {
                    option->enabled = FALSE;
                }
                option->next = NULL;
                if ( entry->data.radio_prefs.tail_option ) {
                    entry->data.radio_prefs.tail_option->next = option;
                } else {
                    entry->data.radio_prefs.options = option;
                }
                entry->data.radio_prefs.tail_option = option;
            }
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
    int length;
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
    line[0] = 0;
    length = 0;
    while ( fgets(&line[length], sizeof(line)-length, fp) ) {
        line[strlen(line)-1] = '\0';
        ++lineno;
        if ( line[strlen(line)-1] == '\\' ) {
            length = strlen(line)-1;
            continue;
        }

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

        if ( strcasecmp(args[0], "LABEL") == 0 ) {
            if ( nargs == 2 ) {
                add_label_option(args);
            } else {
                fprintf(stderr, "Line %d: LABEL requires an argument\n",lineno);
            }
        } else
        if ( strcasecmp(args[0], "SEPARATOR") == 0 ) {
            add_separator_option(args);
        } else
        if ( strcasecmp(args[0], "BOOL") == 0 ) {
            if ( nargs == 5 ) {
                add_bool_option(args);
            } else {
                fprintf(stderr, "Line %d: BOOL requires 4 arguments\n", lineno);
            }
        } else
        if ( strcasecmp(args[0], "RADIO") == 0 ) {
            if ( nargs >= 7 ) {
                add_radio_option(args);
            } else {
                fprintf(stderr, "Line %d: RADIO requires more arguments\n", lineno);
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

        length = 0;
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
            case LABEL_PREF:
                fprintf(fp, "LABEL \"%s\"\n", entry->data.bool_prefs.label);
                break;
            case SEPARATOR_PREF:
                fprintf(fp, "SEPARATOR\n");
                break;
            case BOOL_PREF:
                fprintf(fp, "BOOL \"%s\" \"%s\" \"%s\" %s\n",
                    entry->data.bool_prefs.label,
                    entry->data.bool_prefs.true_option,
                    entry->data.bool_prefs.false_option,
                    entry->data.bool_prefs.value ? "TRUE" : "FALSE");
                break;
            case RADIO_PREF:
                fprintf(fp, "RADIO \"%s\" %s \\\n",
                    entry->data.radio_prefs.label,
                    entry->data.radio_prefs.enabled ? 
                    (entry->data.radio_prefs.enabled == TRUE*2 ? 
                        "ALWAYS" : "TRUE") : "FALSE");
                { struct radio_option *option;
                    for ( option = entry->data.radio_prefs.options;
                          option;
                          option = option->next ) {
                        fprintf(fp, "OPTION \"%s\" \"%s\" %s",
                            option->label, option->value,
                            option->enabled ? "TRUE" : "FALSE");
                        if ( option->next ) {
                            fprintf(fp, " \\\n");
                        } else {
                            fprintf(fp, "\n");
                        }
                    }
                }
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
                case LABEL_PREF:
                    break;
                case SEPARATOR_PREF:
                    break;
                case BOOL_PREF:
                    strcat(command, " ");
                    if ( entry->data.bool_prefs.value ) {
                        strcat(command, entry->data.bool_prefs.true_option);
                    } else {
                        strcat(command, entry->data.bool_prefs.false_option);
                    }
                    break;
                case RADIO_PREF:
                    if ( entry->data.radio_prefs.enabled ) {
                        struct radio_option *option;
                        for ( option = entry->data.radio_prefs.options;
                              option;
                              option = option->next ) {
                            if ( option->enabled ) {
                                strcat(command, " ");
                                strcat(command, option->value);
                            }
                        }
                    }
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
static GladeXML *readme_glade = NULL;
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

static gboolean load_file( GtkText *widget, GdkFont *font, const char *file )
{
    FILE *fp;
    int pos;
    
    gtk_editable_delete_text(GTK_EDITABLE(widget), 0, -1);
    fp = fopen(file, "r");
    if ( fp ) {
        char line[BUFSIZ];
        pos = 0;
        while ( fgets(line, BUFSIZ-1, fp) ) {
            gtk_text_insert(widget, font, NULL, NULL, line, strlen(line));
        }
        fclose(fp);
    }
    gtk_editable_set_position(GTK_EDITABLE(widget), 0);

    return (fp != NULL);
}

void view_readme_slot( GtkWidget* w, gpointer data )
{
    GtkWidget *readme;
    GtkWidget *widget;
    char readme_file[PATH_MAX];
    
    if ( ! readme_glade ) {
        readme_glade = glade_xml_new(PREFS_GLADE, "readme_dialog");
        glade_xml_signal_autoconnect(readme_glade);
        readme = glade_xml_get_widget(readme_glade, "readme_dialog");
        widget = glade_xml_get_widget(readme_glade, "readme_area");
        if ( readme && widget ) {
            gtk_widget_hide(readme);
            sprintf(readme_file, "demos/%s/README", product);
            load_file(GTK_TEXT(widget), NULL, readme_file);
            gtk_widget_show(readme);
        }
    }
}

void close_readme_slot( GtkWidget* w, gpointer data )
{
    GtkWidget *widget;
    
    if ( readme_glade ) {
        widget = glade_xml_get_widget(readme_glade, "readme_dialog");
        if ( widget ) {
            gtk_widget_hide(widget);
        }
        gtk_object_unref(GTK_OBJECT(readme_glade));
        readme_glade = NULL;
    }
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
    close_file_dialog();
    close_readme_slot(NULL, NULL);
}

void cancel_button_slot(GtkWidget *w, gpointer data)
{
    gtk_main_quit();
}

void save_button_slot(GtkWidget *w, gpointer data)
{
    if ( save_prefs(product) < 0 ) {
        quit_ui();
        { char msg[128];
            sprintf(msg, "Unable to save preferences for %s", product);
            message(msg);
        }
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

static void radio_toggle_option( GtkWidget* widget, gpointer func_data)
{
    struct prefentry *entry = (struct prefentry *)func_data;
    struct radio_option *option;

    if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ) {
        entry->data.radio_prefs.enabled = TRUE;
    } else {
        entry->data.radio_prefs.enabled = FALSE;
    }
    for ( option = entry->data.radio_prefs.options;
          option;
          option = option->next ) {
        gtk_widget_set_sensitive(option->button, 
                                 entry->data.radio_prefs.enabled);
    }
}

static void radio_toggle_option_element( GtkWidget* widget, gpointer func_data)
{
    struct radio_option *option = (struct radio_option *)func_data;

    if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ) {
        option->enabled = TRUE;
    } else {
        option->enabled = FALSE;
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
            case LABEL_PREF:
                widget = gtk_label_new(entry->data.label_prefs.label);
                gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);
                gtk_misc_set_alignment(GTK_MISC(widget), 0, .5);
                gtk_widget_show(widget);
                break;

            case SEPARATOR_PREF:
                widget = gtk_hseparator_new();
                gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);
                gtk_widget_show(widget);
                break;

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

            case RADIO_PREF:
                /* Create the toggle button or label itself */
                if ( entry->data.radio_prefs.enabled == TRUE*2 ) {
                    widget = gtk_label_new(entry->data.radio_prefs.label);
                    gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);
                    gtk_misc_set_alignment(GTK_MISC(widget), 0, .5);
                } else {
                    widget = gtk_check_button_new_with_label(
                                entry->data.radio_prefs.label);
                    gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);
                    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
                                entry->data.radio_prefs.enabled);
                    gtk_signal_connect(GTK_OBJECT(widget), "toggled",
                        GTK_SIGNAL_FUNC(radio_toggle_option), (gpointer)entry);
                }
                gtk_widget_show(widget);
                { struct radio_option *option;
                  GSList *radio_list = NULL;
                    for ( option = entry->data.radio_prefs.options;
                          option;
                          option = option->next ) {
                        /* Create an hbox for this line */
                        hbox = gtk_hbox_new(FALSE, 4);
                        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
                        gtk_widget_show(hbox);

                        /* Add a spacing label */
                        widget = gtk_label_new("  ");
                        gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
                        gtk_widget_show(widget);

                        /* Add the radio button itself */
                
                        widget = gtk_radio_button_new_with_label(radio_list, option->label);
                        gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
                        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), option->enabled);
                        gtk_signal_connect(GTK_OBJECT(widget), "toggled",
                            GTK_SIGNAL_FUNC(radio_toggle_option_element),
                                            (gpointer)option);
                        radio_list = gtk_radio_button_group(GTK_RADIO_BUTTON(widget));
                        option->button = widget;
                        gtk_widget_show(widget);
                    }
                }
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
        { char msg[128];
            sprintf(msg, "Unable to load preferences for %s", product);
            message(msg);
        }
        return(-1);
    }
    fill_ui();
    gtk_main();
    quit_ui();
    return(0);
}
