// OpenFileCommand.c

#ifdef windows

  OPENFILENAME OpenFileName = {sizeof(OpenFileName), 0};

  void OpenFileDialog()
  {
    OpenFileName.lpstrFile   = CurrentFileName;
    OpenFileName.nMaxFile    = countof(CurrentFileName);
    OpenFileName.Flags       = OFN_FILEMUSTEXIST;
    OpenFileName.lpstrFilter = "All files\0*.*\0*.bin\0*.bin\0\0";
    if (!GetOpenFileName(&OpenFileName)) {CurrentFileName[0] = 0;}
  }

#else

  #include <gtk/gtk.h>

  static void Activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
      "Open File", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN,
      "Cancel", GTK_RESPONSE_CANCEL,
      "Open",   GTK_RESPONSE_ACCEPT,
      NULL
    );

    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    if (result == GTK_RESPONSE_ACCEPT) {
      char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
      strncpy(CurrentFileName, filename, sizeof(CurrentFileName)-1);
      CurrentFileName[sizeof(CurrentFileName)-1] = 0;
      g_free(filename);
      Wsl(CurrentFileName);
    }
    gtk_widget_destroy(dialog);
    gtk_widget_destroy(window);
  }

  void OpenFileCommand() {
    GtkApplication *App = gtk_application_new("com.dcwbrown.dwdebug", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(App, "activate", G_CALLBACK(Activate), NULL);
    g_application_run(G_APPLICATION(App), 0,0);
    g_object_unref(App);
  }


#endif

void TrimTrailingSpace(char *s) {
  char *p = 0; // First char of potential trailing space
  while (*s) {if (*s > ' ') {p = 0;} else if (!p)  {p = s;}; s++;}
  if (p) {*p=0;}
}

void OpenFileCommand() {

  if (CurrentFile) {CloseHandle(CurrentFile); CurrentFile = 0; CurrentFileName[0] = 0;}

  Sb(); if (Eoln()) {
    OpenFileDialog();
  } else {
    ReadWhile(NotEoln, CurrentFileName, sizeof(CurrentFileName));
    TrimTrailingSpace(CurrentFileName);
  }

  if (CurrentFileName[0]) {

    CurrentFile = CreateFile(CurrentFileName, GENERIC_READ, 0,0, OPEN_EXISTING, 0,0);

    if (CurrentFile == INVALID_HANDLE_VALUE) {
      DWORD winError = GetLastError();
      Ws("Couldn't open ");
      Ws(CurrentFileName);
      Ws(": ");
      WWinError(winError);
      Fail("");
    }

    LoadFile();
  }
}