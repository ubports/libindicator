
#include <libindicator/indicator-ng.h>

static void
indicator_ng_test_func (gconstpointer user_data)
{
  GTestFunc test_func = user_data;
  GTestDBus *bus;

  bus = g_test_dbus_new (G_TEST_DBUS_NONE);
  g_test_dbus_add_service_dir (bus, BUILD_DIR);
  g_test_dbus_up (bus);

  test_func ();

  g_test_dbus_down (bus);
  g_object_unref (bus);
}

#define indicator_ng_test_add(name, test_func) \
  g_test_add_data_func ("/indicator-ng/" name, test_func, indicator_ng_test_func)

static gboolean
stop_main_loop (gpointer user_data)
{
  GMainLoop *loop = user_data;

  g_main_loop_quit (loop);

  return FALSE;
}

static void
test_non_existing (void)
{
  IndicatorNg *indicator;
  GError *error = NULL;

  indicator = indicator_ng_new (SRCDIR "/com.canonical.does.not.exist.indicator", &error);
  g_assert (indicator == NULL);
  g_assert_error (error, G_FILE_ERROR, G_FILE_ERROR_NOENT);

  g_clear_error (&error);
}

static void
test_instantiation (void)
{
  IndicatorNg *indicator;
  GError *error = NULL;
  GMainLoop *loop;

  indicator = indicator_ng_new (SRCDIR "/com.canonical.test.nosuchservice.indicator", &error);
  g_assert (indicator);
  g_assert (error == NULL);

  g_assert_cmpstr (indicator_ng_get_service_file (indicator), ==, SRCDIR "/com.canonical.test.nosuchservice.indicator");
  g_assert_cmpstr (indicator_ng_get_profile (indicator), ==, "desktop");

  loop = g_main_loop_new (NULL, FALSE);
  g_timeout_add (200, stop_main_loop, loop);
  g_main_loop_run (loop);

  /* no such service, so there shouldn't be any indicators */
  g_assert (indicator_object_get_entries (INDICATOR_OBJECT (indicator)) == NULL);

  g_main_loop_unref (loop);
  g_object_unref (indicator);
}

static void
test_instantiation_with_profile (void)
{
  IndicatorNg *indicator;
  GError *error = NULL;

  indicator = indicator_ng_new_for_profile (SRCDIR "/com.canonical.test.indicator", "greeter", &error);
  g_assert (indicator);
  g_assert (error == NULL);

  g_assert_cmpstr (indicator_ng_get_profile (indicator), ==, "greeter");

  g_object_unref (indicator);
}

static void
test_menu (void)
{
  IndicatorNg *indicator;
  GError *error = NULL;
  GMainLoop *loop;
  GList *entries;
  IndicatorObjectEntry *entry;

  indicator = indicator_ng_new (SRCDIR "/com.canonical.test.indicator", &error);
  g_assert (indicator);
  g_assert (error == NULL);

  loop = g_main_loop_new (NULL, FALSE);
  g_timeout_add (500, stop_main_loop, loop);
  g_main_loop_run (loop);

  entries = indicator_object_get_entries (INDICATOR_OBJECT (indicator));
  g_assert_cmpint (g_list_length (entries), ==, 1);

  entry = entries->data;
  g_assert_cmpstr (entry->name_hint, ==, "indicator-test");
  g_assert_cmpstr (entry->accessible_desc, ==, "Test indicator");
  g_assert_cmpstr (gtk_label_get_label (entry->label), ==, "Test");
  {
    GIcon *icon;
    gchar *iconstr;

    gtk_image_get_gicon (entry->image, &icon, NULL);
    g_assert (icon);

    iconstr = g_icon_to_string (icon);
    g_assert_cmpstr (iconstr, ==, "indicator-test");

    g_free (iconstr);
  }
  {
    GList *children;
    GtkMenuItem *item;

    g_assert (entry->menu != NULL);

    children = gtk_container_get_children (GTK_CONTAINER (entry->menu));
    g_assert_cmpint (g_list_length (children), ==, 1);

    item = children->data;
    g_assert (GTK_IS_MENU_ITEM (item));
    g_assert (gtk_widget_is_sensitive (GTK_WIDGET (item)));
    g_assert_cmpstr (gtk_menu_item_get_label (item), ==, "Show");

    g_list_free (children);
  }

  g_list_free (entries);
  g_main_loop_unref (loop);
  g_object_unref (indicator);
}

int
main (int argc, char **argv)
{
  /* gvfs, dconf, and appmenu-gtk leak GDbusConnections, which confuses
   * g_test_dbus_down.  Make sure we're not using any of those.
   */
  g_setenv ("GIO_USE_VFS", "local", TRUE);
  g_setenv ("GSETTINGS_BACKEND", "memory", TRUE);
  g_unsetenv ("UBUNTU_MENUPROXY");

  g_test_init (&argc, &argv, NULL);
  gtk_init (&argc, &argv);

  indicator_ng_test_add ("non-existing", test_non_existing);
  indicator_ng_test_add ("instantiation", test_instantiation);
  indicator_ng_test_add ("instantiation-with-profile", test_instantiation_with_profile);
  indicator_ng_test_add ("menu", test_menu);

  return g_test_run ();
}
