extern const string HUD_GTK_DATADIR;

namespace HudGtk {
	public class CellRendererVariant : Gtk.CellRendererText {
		public Variant value {
			set {
				if (value != null) {
					text = value.print (false);
				} else {
					text = "(null)";
				}
			}
		}
	}

	class Window : Gtk.ApplicationWindow {
		Gtk.Label voice_label;
		Gtk.ListStore model;
		Gtk.ListStore appstack_model;
		Gtk.Entry entry;
		HudClient.Query query;
		
		void results_row_added (Dee.Model results, Dee.ModelIter result_iter) {
			var pos = results.get_position(result_iter);

			Gtk.TreeIter iter;
			model.insert(out iter, (int)pos);

			model.set(iter, 0, query.results_get_command_name(result_iter));
			model.set(iter, 1, query.results_get_description(result_iter));
			model.set(iter, 2, query.results_get_shortcut(result_iter));
			/* Distance isn't in the API because it's internal, but this
			   is a debugging tool so we're leavinging here */
			model.set(iter, 3, results.get_uint32(result_iter, 6));
			model.set(iter, 4, Markup.escape_text(query.results_get_command_id(result_iter).print(true)));
			model.set(iter, 5, query.results_get_command_id(result_iter));
		}

		void results_row_removed (Dee.Model results, Dee.ModelIter result_iter) {
			var pos = results.get_position(result_iter);

			string spath = "%d";
			spath = spath.printf(pos);
			Gtk.TreePath path = new Gtk.TreePath.from_string(spath);
			Gtk.TreeIter iter;
			model.get_iter(out iter, path);
#if VALA_0_36
			model.remove(ref iter);
#else
			model.remove(iter);
#endif
		}

		void appstack_results_row_added (Dee.Model results, Dee.ModelIter result_iter) {
			var pos = results.get_position(result_iter);

			Gtk.TreeIter iter;
			appstack_model.insert(out iter, (int)pos);

			appstack_model.set(iter, 0, query.appstack_get_app_id(result_iter));
			appstack_model.set(iter, 1, query.appstack_get_app_icon(result_iter));
		}

		void appstack_results_row_removed (Dee.Model results, Dee.ModelIter result_iter) {
			var pos = results.get_position(result_iter);

			string spath = "%d";
			spath = spath.printf(pos);
			Gtk.TreePath path = new Gtk.TreePath.from_string(spath);
			Gtk.TreeIter iter;
			appstack_model.get_iter(out iter, path);
#if VALA_0_36
			appstack_model.remove(ref iter);
#else
			appstack_model.remove(iter);
#endif
		}

		void entry_text_changed (Object object, ParamSpec pspec) {
			query.set_query(entry.text);
		}

		void voice_pressed (Gtk.Button button) {
			query.voice_query();
		}

		void view_activated (Gtk.TreeView view, Gtk.TreePath path, Gtk.TreeViewColumn column) {
			Gtk.TreeIter iter;
			Variant key;

			model.get_iter (out iter, path);
			model.get (iter, 5, out key);

			query.execute_command(key, 0);
		}
		
		void voice_query_loading (HudClient.Query proxy) {
			debug("Voice query is loading");
			voice_label.label = "Loading";
		}
		
		void voice_query_failed (HudClient.Query proxy, string cause) {
			debug("Voice query failed, cause=[%s]", cause);
			voice_label.label = "Idle";
			var dialog = new Gtk.MessageDialog(null,Gtk.DialogFlags.MODAL,Gtk.MessageType.ERROR, Gtk.ButtonsType.OK, "Voice query failed, cause=[%s]", cause);
			dialog.set_title("Voice query failed");
			dialog.run();
			dialog.destroy();
		}
		
		void voice_query_listening (HudClient.Query proxy) {
			debug("Voice query is listening");
			voice_label.label = "Listening";
		}
		
		void voice_query_heard_something (HudClient.Query proxy) {
			debug("Voice query has heard something");
			voice_label.label = "Heard Something";
		}
		
		void voice_query_finished (HudClient.Query proxy, string query) {
			debug("Voice query is finished, query=[%s]", query);
			voice_label.label = "Idle";
			entry.text = query;
		}

		void appstack_view_activated (Gtk.TreeView view, Gtk.TreePath path, Gtk.TreeViewColumn column) {
			Gtk.TreeIter iter;
			string key;

			appstack_model.get_iter (out iter, path);
			appstack_model.get (iter, 0, out key);
			
			query.set_appstack_app(key);
		}

		public Window (Gtk.Application application) {
			Object (application: application, title: "Hud");
			set_default_size (500, 300);
			
			query = new HudClient.Query("");
			query.models_changed.connect ( models_changed );
    }			
			
		void models_changed(HudClient.Query proxy) {
		  query.models_changed.disconnect ( models_changed );
		  
			var builder = new Gtk.Builder ();
			try {
				new CellRendererVariant ();
				builder.add_from_file (HUD_GTK_DATADIR + "/hud-gtk.ui");
			} catch (Error e) {
				error (e.message);
			}

			voice_label = builder.get_object ("voice-status") as Gtk.Label;
			query.voice_query_loading.connect ( voice_query_loading );
			query.voice_query_failed.connect ( voice_query_failed );
			query.voice_query_listening.connect ( voice_query_listening );
			query.voice_query_heard_something.connect ( voice_query_heard_something );
			query.voice_query_finished.connect ( voice_query_finished );
			
			Dee.Model results = query.get_results_model();
			results.row_added.connect (results_row_added);
			results.row_removed.connect (results_row_removed);

			Dee.Model appstack_results = query.get_appstack_model();
			appstack_results.row_added.connect (appstack_results_row_added);
			appstack_results.row_removed.connect (appstack_results_row_removed);

			entry = builder.get_object ("entry") as Gtk.Entry;
			model = builder.get_object ("liststore") as Gtk.ListStore;
			entry.notify["text"].connect (entry_text_changed);
			(builder.get_object ("voice") as Gtk.Button).clicked.connect (voice_pressed);
			(builder.get_object ("treeview") as Gtk.TreeView).row_activated.connect (view_activated);
			add (builder.get_object ("grid") as Gtk.Widget);
			
			appstack_model = builder.get_object ("appstack_liststore") as Gtk.ListStore;
			(builder.get_object ("appstack_treeview") as Gtk.TreeView).row_activated.connect (appstack_view_activated);
		}
	}

	class Application : Gtk.Application {
		protected override void activate () {
			new Window (this).show_all ();
		}

		public Application () {
			Object (application_id: "com.canonical.HudGtk");
		}
	}
}

int main (string[] args) {
	return new HudGtk.Application ().run (args);
}
