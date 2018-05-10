# include <iostream>
# include <stdlib.h>
# include <functional>

# include <boost/filesystem.hpp>
# include <boost/filesystem/operations.hpp>
# include <boost/property_tree/ptree.hpp>
# include <boost/property_tree/json_parser.hpp>
# include <boost/property_tree/ini_parser.hpp>

# include "config.hh"
# include "poll.hh"
# include "utils/utils.hh"

using namespace std;
using namespace boost::filesystem;
using boost::property_tree::ptree;

namespace Astroid {
  Config::Config (bool _test, bool no_load) {
    if (_test) {
      LOG (info) << "cf: loading test config.";
    }

    test = _test;

    load_dirs ();

    std_paths.config_file = std_paths.config_dir / path("config");

    if (!no_load)
      load_config (); // re-sets config_dir to parent of fname
  }

  Config::Config (const char * fname, bool no_load) {
    test = false;

    load_dirs ();
    std_paths.config_file = path(fname);
    LOG (info) << "cf: loading config: " << fname;
    if (!no_load)
      load_config (); // re-sets config_dir to parent of fname
  }

  void Config::load_dirs () {

    if (test) {
      /* using $PWD/test/test_home */

      path cur_path (current_path() );

      std_paths.home = cur_path / path("tests/test_home");

      LOG (debug) << "cf: using home and config_dir directory: " << std_paths.home.c_str ();

    } else {
      char * home_c = getenv ("HOME");
      LOG (debug) << "HOME: " << home_c;

      if (home_c == NULL) {
        LOG (error) << "cf: HOME environment variable not set.";
        exit (1);
      }

      std_paths.home = path(home_c);
    }

    /* default config */
    if (test) {
      std_paths.config_dir = std_paths.home;
    } else {
      char * config_home = getenv ("XDG_CONFIG_HOME");
      if (config_home == NULL) {
        std_paths.config_dir = std_paths.home / path(".config/astroid");
      } else {
        std_paths.config_dir = path(config_home) / path("astroid");
      }
    }

    /* searches file */
    std_paths.searches_file = std_paths.config_dir / path("searches");

    /* plugin dir */
    std_paths.plugin_dir = std_paths.config_dir / path ("plugins");

    /* default data */
    char * data = getenv ("XDG_DATA_HOME");
    if (data == NULL) {
      std_paths.data_dir = std_paths.home / path(".local/share/astroid");
    } else {
      std_paths.data_dir = path(data) / path("astroid");
    }

    /* default cache */
    char * cache = getenv ("XDG_CACHE_HOME");
    if (cache == NULL) {
      std_paths.cache_dir = std_paths.home / path(".cache/astroid");
    } else {
      std_paths.cache_dir = path(cache) / path("astroid");
    }

    /* default runtime */
    char * runtime = getenv ("XDG_RUNTIME_HOME");
    if (runtime == NULL) {
      std_paths.runtime_dir = std_paths.cache_dir;
    } else {
      std_paths.runtime_dir = path(runtime) / path("astroid");
    }

    /* save to and attach from directory */
    std_paths.save_dir   = std_paths.home;
    std_paths.attach_dir = std_paths.home;
  }

  ptree Config::setup_default_config (bool initial) {
    ptree default_config;
    default_config.put ("astroid.config.version", CONFIG_VERSION);
    std::string nm_cfg = path(std_paths.home / path (".notmuch-config")).string();
    default_config.put ("astroid.notmuch_config" , nm_cfg);

    default_config.put ("astroid.debug.dryrun_sending", false);

    /* only show hints with a level higher than this */
    default_config.put ("astroid.hints.level", 0);

    if (initial) {
      /* initial default options - these are only set when a new
       * configuration file is created. */

      /* default example account:
       *
       * note: AccountManager will complain if there are no accounts defined.
       *
       */
      default_config.put ("accounts.charlie.name", "Charlie Root");
      default_config.put ("accounts.charlie.email", "root@localhost");
      default_config.put ("accounts.charlie.gpgkey", "");
      default_config.put ("accounts.charlie.always_gpg_sign", false);
      default_config.put ("accounts.charlie.sendmail", "msmtp -i -t");
      default_config.put ("accounts.charlie.default", true);
      default_config.put ("accounts.charlie.save_sent", false);
      default_config.put ("accounts.charlie.save_sent_to",
          "/home/root/Mail/sent/cur/");
      default_config.put ("accounts.charlie.additional_sent_tags", "");

      default_config.put ("accounts.charlie.save_drafts_to",
          "/home/root/Mail/drafts/");

      default_config.put ("accounts.charlie.signature_separate", false);
      default_config.put ("accounts.charlie.signature_file", "");
      default_config.put ("accounts.charlie.signature_file_markdown", "");
      default_config.put ("accounts.charlie.signature_default_on", true);
      default_config.put ("accounts.charlie.signature_attach", false);

      default_config.put ("accounts.charlie.select_query", "");

      /* default searches, also only set if initial */
      default_config.put("startup.queries.inbox", "tag:inbox");
    }

    /* terminal */
    default_config.put ("terminal.height", 10);
    default_config.put ("terminal.font_description", "default"); // https://developer.gnome.org/pango/stable/pango-Fonts.html#pango-font-description-from-string

    /* thread index */
    default_config.put ("thread_index.page_jump_rows", 6);
    default_config.put ("thread_index.sort_order", "newest");

    default_config.put ("general.time.clock_format", "local"); // or 24h, 12h
    default_config.put ("general.time.same_year", "%b %-e");
    default_config.put ("general.time.diff_year", "%x");

    /* thread index cell theme */
    default_config.put ("thread_index.cell.font_description", "default");
    default_config.put ("thread_index.cell.line_spacing", 2);
    default_config.put ("thread_index.cell.date_length", 10);
    default_config.put ("thread_index.cell.message_count_length", 4);
    default_config.put ("thread_index.cell.authors_length", 20);

    default_config.put ("thread_index.cell.subject_color", "#807d74");
    default_config.put ("thread_index.cell.subject_color_selected", "#000000");
    default_config.put ("thread_index.cell.background_color_selected", "");
    default_config.put ("thread_index.cell.background_color_marked", "#fff584");
    default_config.put ("thread_index.cell.background_color_marked_selected", "#bcb559");

    default_config.put ("thread_index.cell.tags_length", 80);
    default_config.put ("thread_index.cell.tags_upper_color", "#e5e5e5");
    default_config.put ("thread_index.cell.tags_lower_color", "#333333");
    default_config.put ("thread_index.cell.tags_alpha", "0.5");
    default_config.put ("thread_index.cell.hidden_tags", "attachment,flagged,unread");


    /* editor */
    // also useful: '+/^\\s*\\n/' '+nohl'
# ifndef DISABLE_EMBEDDED
    default_config.put ("editor.cmd", "gvim -geom 10x10 --servername %2 --socketid %3 -f -c 'set ft=mail' '+set fileencoding=utf-8' '+set ff=unix' '+set enc=utf-8' '+set fo+=w' %1");
    default_config.put ("editor.external_editor", false); // should be true on Wayland
# else
    default_config.put ("editor.cmd", "gvim -f -c 'set ft=mail' '+set fileencoding=utf-8' '+set enc=utf-8' '+set ff=unix' '+set fo+=w' %1");
    default_config.put ("editor.external_editor", true); // should be true on Wayland
# endif

    default_config.put ("editor.charset", "utf-8");
    default_config.put ("editor.save_draft_on_force_quit", true);

    default_config.put ("editor.attachment_words", "attach");
    default_config.put ("editor.attachment_directory", "~");

    default_config.put ("editor.markdown_processor", "marked");

    /* mail composition */
    default_config.put ("mail.reply.quote_line", "Excerpts from %1's message of %2:"); // %1 = author, %2 = pretty_verbose_date
    default_config.put ("mail.reply.mailinglist_reply_to_sender", true);
    default_config.put ("mail.forward.quote_line", "Forwarding %1's message of %2:"); // %1 = author, %2 = pretty_verbose_date
    default_config.put ("mail.forward.disposition", "inline");
    default_config.put ("mail.sent_tags", "sent");
    default_config.put ("mail.message_id_fqdn", ""); // custom fqdn for the message id: default: local hostname
    default_config.put ("mail.message_id_user", ""); // custom user for the message id: default: 'astroid'
    default_config.put ("mail.user_agent", "default");
    default_config.put ("mail.send_delay", 2); // wait seconds before sending, allowing to cancel
    default_config.put ("mail.close_on_success", false); // close page automatically on succesful sending of message
    default_config.put ("mail.format_flowed", false); // mail sent with astroid can be reformatted using format_flowed

    /* polling */
    default_config.put ("poll.interval", Poll::DEFAULT_POLL_INTERVAL); // seconds
    default_config.put ("poll.always_full_refresh", false); // always do full refresh after poll, slow.

    /* attachments
     *
     *   a chunk is saved and opened with the this command */
    default_config.put ("attachment.external_open_cmd", "xdg-open");

    /* thread view
     *
     *   if true; chunks (parts) that are not viewed initially are opened
     *            externally when this is set. the part is opened with
     *            'attachment.external_open_cmd'. */
    default_config.put ("thread_view.open_html_part_external", false);

    /*   if a link is clicked (html, ftp, etc..) it is executed with this
     *   command. */
    default_config.put ("thread_view.open_external_link", "xdg-open");

    default_config.put ("thread_view.default_save_directory", "~");

    default_config.put ("thread_view.indent_messages", false);

    /* code prettify */
    default_config.put ("thread_view.code_prettify.enable", true);

    // a comma-separeted list of tags which code_prettify is enabled for, if empty,
    // allow for all messages.
    default_config.put ("thread_view.code_prettify.for_tags", "");

    // the tag enclosing code
    default_config.put ("thread_view.code_prettify.code_tag", "```");
    default_config.put ("thread_view.code_prettify.enable_for_patches", true);

    /* gravatar */
    default_config.put ("thread_view.gravatar.enable", true);

    /* mark unread */
    default_config.put ("thread_view.mark_unread_delay", .5);

    /* expand flagged messages by default */
    default_config.put ("thread_view.expand_flagged", true);

    /* crypto */
    default_config.put ("crypto.gpg.path", "gpg2");
    default_config.put ("crypto.gpg.always_trust", true);
    default_config.put ("crypto.gpg.throw_keyids", true);

    /* saved searches */
    default_config.put ("saved_searches.show_on_startup", false);
    default_config.put ("saved_searches.save_history", true);
    default_config.put ("saved_searches.history_lines_to_show", 15); /* -1 is all */
    default_config.put ("saved_searches.history_lines", 1000); /* number of history lines to store */

    return default_config;
  }

  void Config::write_back_config () {
    LOG (warn) << "cf: writing back config to: " << std_paths.config_file;

    write_json (std_paths.config_file.c_str (), config);
  }

  void Config::load_config (bool initial) {
    if (test) {
      LOG (info) << "cf: test config, loading defaults.";
      config = setup_default_config (true);
      config.put ("poll.interval", 0);
      config.put ("accounts.charlie.gpgkey", "gaute@astroidmail.bar");
      config.put ("mail.send_delay", 0);
      std::string test_nmcfg_path = path(current_path() / path ("tests/mail/test_config")).string();
      boost::property_tree::read_ini (test_nmcfg_path, notmuch_config);
      has_notmuch_config = true;
      return;
    }

    LOG (info) << "cf: loading: " << std_paths.config_file;

    std_paths.config_dir = absolute(std_paths.config_file.parent_path());
    if (!is_directory(std_paths.config_dir)) {
      LOG (warn) << "cf: making config dir..";
      create_directories (std_paths.config_dir);
    }

    if (!is_directory(std_paths.runtime_dir)) {
      LOG (warn) << "cf: making runtime dir..";
      create_directories (std_paths.runtime_dir);
    }

    if (!is_regular_file (std_paths.config_file)) {
      if (!initial) {
        LOG (warn) << "cf: no config, using defaults.";
      }
      config = setup_default_config (true);
      write_back_config ();
    } else {

      /* loading config file */
      ptree new_config;
      config = setup_default_config (false);
      read_json (std_paths.config_file.c_str(), new_config);
      LOG (info) << "cf: version: " << config.get<int>("astroid.config.version");

      merge_ptree (new_config);

      if (!check_config (new_config)) {
        write_back_config ();
      }
    }

    /* load save_dir */
    std_paths.save_dir = Utils::expand(bfs::path (config.get<string>("thread_view.default_save_directory")));
    run_paths.save_dir = std_paths.save_dir;

    /* load attach_dir */
    std_paths.attach_dir = Utils::expand(bfs::path (config.get<string>("editor.attachment_directory")));
    run_paths.attach_dir = std_paths.attach_dir;

    /* read notmuch config */
    if (is_regular_file (config.get<std::string> ("astroid.notmuch_config"))) {
      boost::property_tree::read_ini (
        config.get<std::string> ("astroid.notmuch_config"),
        notmuch_config);
      has_notmuch_config = true;
    } else {
      has_notmuch_config = false;
    }
  }


  bool Config::check_config (ptree new_config) {
    bool changed = false;

    if (new_config != config) {
      changed = true;
    }

    int version = config.get<int>("astroid.config.version");

    if (version < 1) {
      /* check accounts for save_draft */
      ptree apt = config.get_child ("accounts");

      for (auto &kv : apt) {
        try {

          ustring sto = kv.second.get<string> ("save_drafts_to");

        } catch (const boost::property_tree::ptree_bad_path &ex) {

          LOG (warn) << "config: setting default save draft path for account: " << kv.first << ", please update!";
          ustring key = ustring::compose ("accounts.%1.save_drafts_to", kv.first);
          config.put (key.c_str (), "/home/root/Mail/drafts/");

        }
      }

      changed = true;
    }

    if (version < 2) {
      /* check accounts additional_sent_tags */
      ptree apt = config.get_child ("accounts");

      for (auto &kv : apt) {
        try {

          ustring sto = kv.second.get<string> ("additional_sent_tags");

        } catch (const boost::property_tree::ptree_bad_path &ex) {

          ustring key = ustring::compose ("accounts.%1.additional_sent_tags", kv.first);
          config.put (key.c_str (), "");
        }
      }

      changed = true;
    }

    if (version < 3) {
      LOG (warn) << "config: 'astroid.notmuch.sent_tags' have been moved to 'mail.sent_tags'";

      config.put ("mail.sent_tags", config.get<string>("astroid.notmuch.sent_tags"));
      config.erase ("astroid.notmuch.sent_tags");

      changed = true;

      LOG (warn) << "config: astroid now reads standard notmuch options from notmuch config, it is configured through: 'astroid.notmuch_config' and is now set to the default: ~/.notmuch-config. please validate!";
    }

    if (version < 10) {
      /* check accounts signature */
      ptree apt = config.get_child ("accounts");

      for (auto &kv : apt) {
        if (version < 5) {
          try {

            ustring sto = kv.second.get<string> ("signature_file");

          } catch (const boost::property_tree::ptree_bad_path &ex) {

            ustring key = ustring::compose ("accounts.%1.signature_file", kv.first);
            config.put (key.c_str (), "");
          }

          try {

            ustring sto = kv.second.get<string> ("signature_default_on");

          } catch (const boost::property_tree::ptree_bad_path &ex) {

            ustring key = ustring::compose ("accounts.%1.signature_default_on", kv.first);
            config.put (key.c_str (), true);
          }

          try {

            ustring sto = kv.second.get<string> ("signature_attach");

          } catch (const boost::property_tree::ptree_bad_path &ex) {

            ustring key = ustring::compose ("accounts.%1.signature_attach", kv.first);
            config.put (key.c_str (), false);
          }
        }

        if (version < 6) {
          try {

            ustring sto = kv.second.get<string> ("always_gpg_sign");

          } catch (const boost::property_tree::ptree_bad_path &ex) {

            ustring key = ustring::compose ("accounts.%1.always_gpg_sign", kv.first);
            config.put (key.c_str (), false);
          }
        }

        if (version < 8) {
          try {

            ustring ss = kv.second.get<string> ("signature_separate");

          } catch (const boost::property_tree::ptree_bad_path &ex) {

            ustring key = ustring::compose ("accounts.%1.signature_separate", kv.first);
            config.put (key.c_str (), false);
          }
        }

        if (version < 9) {
          try {

            ustring ss = kv.second.get<string> ("select_query");

          } catch (const boost::property_tree::ptree_bad_path &ex) {

            ustring key = ustring::compose ("accounts.%1.select_query", kv.first);
            config.put (key.c_str (), "");
          }
        }

        if (version < 10) {
          try {

            ustring sto = kv.second.get<string> ("signature_file_markdown");

          } catch (const boost::property_tree::ptree_bad_path &ex) {

            ustring key = ustring::compose ("accounts.%1.signature_file_markdown", kv.first);
            config.put (key.c_str (), "");
          }
        }
      }

      changed = true;
    }

    /* check deprecated keys (as of version 3) */
    try {
      config.get<string> ("astroid.notmuch.db");

      LOG (error) << "config: option 'astroid.notmuch.db' is deprecated, it is read from notmuch config.";
    } catch (const boost::property_tree::ptree_bad_path &ex) { }


    try {
      config.get<string> ("astroid.notmuch.excluded_tags");
      LOG (error) << "config: option 'astroid.notmuch.excluded_tags' is deprecated, it is read from notmuch config.";
    } catch (const boost::property_tree::ptree_bad_path &ex) { }

    try {
      config.get<string> ("astroid.notmuch.sent_tags");
      LOG (error) << "config: option 'astroid.notmuch.sent_tags' is deprecated, it is moved to 'mail.sent_tags'.";
    } catch (const boost::property_tree::ptree_bad_path &ex) { }

    /* generalized editor */
    if (version < 4) {
      try {
        string gvim = config.get<string> ("editor.gvim.cmd");
        string args = config.get<string> ("editor.gvim.args");

        string new_gvim = gvim + " -geom 10x10 --servername %2 --socketid %3 " + args + " %1";


        LOG (warn) << "config: editor has been generalized, editor.cmd replaces gvim.cmd and gvim.args.";

        config.put<string> ("editor.cmd", new_gvim);

        changed = true;
      } catch (const boost::property_tree::ptree_bad_path &ex) { }
    }

    try {
      string gvim = config.get<string> ("editor.gvim.cmd");
      string args = config.get<string> ("editor.gvim.args");

      LOG (warn) << "editor.gvim.cmd and editor.gvim.args are replaced by editor.cmd, and may be removed.";
    } catch (const boost::property_tree::ptree_bad_path &ex) { }

    if (version < CONFIG_VERSION) {
      config.put ("astroid.config.version", CONFIG_VERSION);
      changed = true;
    }

    if (changed) {
      LOG (warn) << "cf: missing values in config have been updated with defaults (old version: " << version << ", new: " << CONFIG_VERSION << ")";
    }

    return !changed;
  }

  /* TODO: split into utils/ somewhere.. */
  /* merge of property trees */

  // from http://stackoverflow.com/questions/8154107/how-do-i-merge-update-a-boostproperty-treeptree
  template<typename T>
    void Config::traverse_recursive(
        const boost::property_tree::ptree &parent,
        const boost::property_tree::ptree::path_type &childPath,
        const boost::property_tree::ptree &child, T &method)
    {
      using boost::property_tree::ptree;

      method(parent, childPath, child);
      for(ptree::const_iterator it=child.begin();
          it!=child.end();
          ++it) {
        ptree::path_type curPath = childPath / ptree::path_type(it->first);
        traverse_recursive(parent, curPath, it->second, method);
      }
    }

  void Config::traverse(const ptree &parent,
            function<void(const ptree &,
            const ptree::path_type &,
            const ptree&)> method)
    {
      traverse_recursive(parent, "", parent, method);
    }

  void Config::merge(const ptree & /* parent */,
             const ptree::path_type &childPath,
             const ptree &child) {

    // overwrites existing default values
    config.put(childPath, child.data());
  }

  void Config::merge_ptree(const ptree &pt)   {
    function<void(const ptree &,
      const ptree::path_type &,
      const ptree&)> method = bind (&Config::merge, this, _1, _2, _3);

    traverse(pt, method);
  }
}

