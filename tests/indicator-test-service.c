
#include <gio/gio.h>

typedef struct
{
  GSimpleActionGroup *actions;
  GMenu *menu;

  guint actions_export_id;
  guint menu_export_id;
} IndicatorTestService;

static void
bus_acquired (GDBusConnection *connection,
              const gchar     *name,
              gpointer         user_data)
{
  IndicatorTestService *indicator = user_data;
  GError *error = NULL;

  indicator->actions_export_id = g_dbus_connection_export_action_group (connection,
                                                                        "/com/canonical/indicator/test",
                                                                        G_ACTION_GROUP (indicator->actions),
                                                                        &error);
  if (indicator->actions_export_id == 0)
    {
      g_warning ("cannot export action group: %s", error->message);
      g_error_free (error);
      return;
    }

  indicator->menu_export_id = g_dbus_connection_export_menu_model (connection,
                                                                   "/com/canonical/indicator/test/desktop",
                                                                   G_MENU_MODEL (indicator->menu),
                                                                   &error);
  if (indicator->menu_export_id == 0)
    {
      g_warning ("cannot export menu: %s", error->message);
      g_error_free (error);
      return;
    }
}

static void
name_lost (GDBusConnection *connection,
           const gchar     *name,
           gpointer         user_data)
{
  IndicatorTestService *indicator = user_data;

  if (indicator->actions_export_id)
    g_dbus_connection_unexport_action_group (connection, indicator->actions_export_id);

  if (indicator->menu_export_id)
    g_dbus_connection_unexport_menu_model (connection, indicator->menu_export_id);
}

static void
activate_show (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  g_message ("showing");
}

int
main (int argc, char **argv)
{
  IndicatorTestService indicator = { 0 };
  GMenuItem *item;
  GMenu *submenu;
  GActionEntry entries[] = {
    { "_header", NULL, NULL, "('Test', 'indicator-test', 'Test indicator', true)", NULL },
    { "show", activate_show, NULL, NULL, NULL }
  };
  GMainLoop *loop;

  indicator.actions = g_simple_action_group_new ();
  g_simple_action_group_add_entries (indicator.actions, entries, G_N_ELEMENTS (entries), NULL);

  submenu = g_menu_new ();
  g_menu_append (submenu, "Show", "indicator.show");
  item = g_menu_item_new (NULL, "indicator._header");
  g_menu_item_set_submenu (item, G_MENU_MODEL (submenu));
  indicator.menu = g_menu_new ();
  g_menu_append_item (indicator.menu, item);

  g_bus_own_name (G_BUS_TYPE_SESSION,
                  "com.canonical.indicator.test",
                  G_BUS_NAME_OWNER_FLAGS_NONE,
                  bus_acquired,
                  NULL,
                  name_lost,
                  &indicator,
                  NULL);

  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);

  g_object_unref (submenu);
  g_object_unref (item);
  g_object_unref (indicator.actions);
  g_object_unref (indicator.menu);
  g_object_unref (loop);

  return 0;
}
