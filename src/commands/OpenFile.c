// OpenFile.c

// Current loaded file

FileHandle CurrentFile          = 0;
char       CurrentFilename[500] = "";



#ifdef windows




  OPENFILENAME OpenFileName = {sizeof(OpenFileName), 0};

  void OpenFileDialog(void)
  {
    OpenFileName.lpstrFile   = CurrentFilename;
    OpenFileName.nMaxFile    = countof(CurrentFilename);
    OpenFileName.Flags       = OFN_FILEMUSTEXIST;
    OpenFileName.lpstrFilter = "All files\0*.*\0*.bin\0*.bin\0\0";
    if (!GetOpenFileName(&OpenFileName)) {CurrentFilename[0] = 0;}
  }




#else // Linux




  typedef enum {G_APPLICATION_FLAGS_NONE = 0}                       GApplicationFlags;
  typedef enum {G_CONNECT_AFTER = 1<<0, G_CONNECT_SWAPPED = 1<<1}   GConnectFlags;
  typedef enum {GTK_FILE_CHOOSER_ACTION_OPEN = 0}                   GtkFileChooserAction;
  typedef enum {GTK_RESPONSE_ACCEPT = -3, GTK_RESPONSE_CANCEL = -6} GtkResponseType;

  typedef struct _GtkApplication GtkApplication;
  typedef struct _GtkWindow      GtkWindow;
  typedef struct _GtkWidget      GtkWidget;
  typedef struct _GtkDialog      GtkDialog;
  typedef struct _GtkFileChooser GtkFileChooser;
  typedef struct _GClosure       GClosure;
  typedef struct _GApplication   GApplication;

  typedef void (*GCallback)(void);
  typedef void (*GClosureNotify)(void *data, GClosure *closure);

  GtkApplication *(*GtkApplicationNew)        (const char *application_id, GApplicationFlags flags);
  GtkWidget *     (*GtkApplicationWindowNew)  (GtkApplication *application);
  unsigned long   (*GSignalConnectData)       (void *instance, const char *detailed_signal,
                                               GCallback c_handler, void *data,
                                               GClosureNotify destroy_data, GConnectFlags connect_flags);
  int             (*GApplicationRun)          (GApplication *app, int argc, char **argv);
  void            (*GObjectUnref)             (void *object);
  GtkWidget *     (*GtkFileChooserDialogNew)  (const char *title, GtkWindow *parent,
                                               GtkFileChooserAction action,
                                               const char *first_button_text,
                                               ...);
  int             (*GtkDialogRun)             (GtkDialog *dialog);
  char *          (*GtkFileChooserGetFilename)(GtkFileChooser *chooser);
  void            (*GFree)                    (void *mem);
  void            (*GtkWidgetDestroy)         (GtkWidget *widget);




  int             GtkPresent = -1;  // -1 = haven't tried yet, 0 = tried but not available, 1 = tried and is available
  GtkApplication *App        = NULL;

  int HaveGtk(void) {
    if (GtkPresent < 0) {
      void *gtk;
      GtkPresent = (    (gtk = dlopen("libgtk-3.so", RTLD_NOW))
                 &&  (GtkApplicationNew         = dlsym(gtk, "gtk_application_new"))
                 &&  (GtkApplicationWindowNew   = dlsym(gtk, "gtk_application_window_new"))
                 &&  (GSignalConnectData        = dlsym(gtk, "g_signal_connect_data"))
                 &&  (GApplicationRun           = dlsym(gtk, "g_application_run"))
                 &&  (GObjectUnref              = dlsym(gtk, "g_object_unref"))
                 &&  (GtkFileChooserDialogNew   = dlsym(gtk, "gtk_file_chooser_dialog_new"))
                 &&  (GtkDialogRun              = dlsym(gtk, "gtk_dialog_run"))
                 &&  (GtkFileChooserGetFilename = dlsym(gtk, "gtk_file_chooser_get_filename"))
                 &&  (GFree                     = dlsym(gtk, "g_free"))
                 &&  (GtkWidgetDestroy          = dlsym(gtk, "gtk_widget_destroy"))
                 &&  (App = GtkApplicationNew("com.dcwbrown.dwdebug", G_APPLICATION_FLAGS_NONE)));
    }
    return GtkPresent;
  }


  static void Activate(GtkApplication *app, void * user_data) {
    GtkWidget *window = GtkApplicationWindowNew(app);
    GtkWidget *dialog = GtkFileChooserDialogNew(
      "Open File", (GtkWindow*)window, GTK_FILE_CHOOSER_ACTION_OPEN,
      "Cancel", GTK_RESPONSE_CANCEL,
      "Open",   GTK_RESPONSE_ACCEPT,
      NULL
    );
    if (GtkDialogRun((GtkDialog*)dialog) == GTK_RESPONSE_ACCEPT) {
      char *filename = GtkFileChooserGetFilename((GtkFileChooser*)dialog);
      strncpy(CurrentFilename, filename, sizeof(CurrentFilename)-1);
      CurrentFilename[sizeof(CurrentFilename)-1] = 0;
      GFree(filename);
    }
    GtkWidgetDestroy(dialog);
    GtkWidgetDestroy(window);
  }

  void OpenFileDialog(void) {
    CurrentFilename[0] = 0;
    if (HaveGtk()) {
      GSignalConnectData(App, "activate", ((GCallback)Activate), NULL, NULL, 0);
      GApplicationRun((GApplication*)App, 0,0);
      GObjectUnref(App);
    } else {
      Wsl("No open file dialog support present. Load command requires a filename parameter.");
    }
  }


#endif
