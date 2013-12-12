#define	_GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

#define PURPLE_PLUGINS

#include <glib.h>

#define PURPLE_PLUGINS

#include "cmds.h"
#include "debug.h"
#include "notify.h"
#include "plugin.h"
#include "version.h"

static char const		me[] = "pidgin-vim";
static PurplePlugin *		plugin_id;
static PurpleCmdId		vim_command_id;
static PurpleCmdId		cat_command_id;

static	struct timespec how_long = {
	/* About 1.5 seconds between lines				 */
	1,
	1000000000 / 2
};

static	int		how_many = 128;

static	void
say(
	PurpleConversation *	conv,	/* Conversation for command	 */
	gchar const *		fmt,	/* Printf-style format		 */
	...				/* Arguments as required	 */
)
{
	if( fmt )	{
		int const	type = purple_conversation_get_type( conv );
		va_list		ap;
		char *		str;

		va_start( ap, fmt );
		if( vasprintf( &str, fmt, ap ) != -1 )	{
			switch( type )	{
			default:
				purple_debug_misc(
					me,
					"huh? %d '%s'\n",
					type,
					str
				);
				break;
			case PURPLE_CONV_TYPE_IM:
				purple_conv_im_send(
					PURPLE_CONV_IM( conv ),
					str
				);
				break;
			case PURPLE_CONV_TYPE_CHAT:
				purple_conv_chat_send(
					PURPLE_CONV_CHAT( conv ),
					str
				);
				break;
			}
			free( str );
		}
		va_end( ap );
	}
}

static	void
do_file(
	PurpleConversation *	conv,	/* Conversation for command	 */
	char const *		fn	/* File we should send		 */
)
{
	do	{
		FILE *		fyle = fopen( fn, "rt" );
		int		n;

		if( ! fyle )	{
			say(
				conv,
				"Cannot open '%s'; errno=%d (%s)",
				fn,
				errno,
				strerror( errno )
			);
			break;
		}
		n = 0;
		while( !feof( fyle ) )	{
			char	buf[ BUFSIZ + 1 ];
			char *	eol;
			size_t	len;

			if( !fgets( buf, sizeof( buf )-1, fyle ) ) {
				break;
			}
			buf[ BUFSIZ ] = '\0';
			eol = buf + strlen( buf );
			while( (eol > buf) && isspace( eol[-1] ) ) {
				*--eol = '\0';
			}
			len = (eol - buf);
			if( (n + len) >= how_many )	{
				(void) nanosleep( &how_long, NULL );
				n = 0;
			}
			say( conv, "%s", buf );
			n += len;
		}
		(void) fclose( fyle );
	} while( 0 );
}

static	PurpleCmdRet
vi_cb(
	PurpleConversation *	conv,	/* Conversation for command	 */
	gchar const *		cmd,	/* The exact command as endered  */
	gchar * *		args,	/* The args passed by the cmd	 */
	gchar * *		error,	/* Any errors that occurred	 */
	void *			data	/* User-defined data		 */
)
{
	PurpleCmdRet		retval;

	retval = PURPLE_CMD_RET_OK;
	do {
		char		tfile[ 32 ];
		char		cmd[ 80 ];
		int		fd;

		strcpy(
			tfile,
			"/tmp/pidgin-tmp.XXXXXX"
		);
		fd = mkstemp( tfile );
		if( fd == -1 )	{
			int const	e = errno;

			say(
				conv,
				"Cannot create '%s'; errno=%d(%s)\n",
				tfile,
				e,
				strerror( e )
			);
			break;
		}
		(void) close( fd );
		sprintf( cmd, "gvim -f '%s'", tfile );
		if( system( cmd ) == -1 )	{
			int const	e = errno;

			say(
				conv,
				"%s; errno=%d(%s)\n",
				cmd,
				e,
				strerror( e )
			);
			break;
		}
		do_file( conv, tfile );
		(void) unlink( tfile );
	} while( 0 );
	return( retval );
}

static	PurpleCmdRet
cat_cb(
	PurpleConversation *	conv,	/* Conversation for command	 */
	gchar const *		cmd,	/* The exact command as endered  */
	gchar * *		args,	/* The args passed by the cmd	 */
	gchar * *		error,	/* Any errors that occurred	 */
	void *			data	/* User-defined data		 */
)
{
	PurpleCmdRet		retval;

	retval = PURPLE_CMD_RET_OK;
	{
		int		i;
		char const *	fn;

		for( i = 0; (fn = args[i++]); )	{
			do_file( conv, fn );
		}
	} while( 0 );
	return( retval );
}

static gboolean
plugin_load(
	PurplePlugin *		plugin
)
{
	plugin_id = plugin;
	/*								 */
	vim_command_id = purple_cmd_register(
		"vi",			/* Command name			 */
		"",			/* Command arg format		 */
		PURPLE_CMD_P_DEFAULT,	/* Command priority		 */
		(PURPLE_CMD_FLAG_IM|PURPLE_CMD_FLAG_CHAT),
		me,			/* Plugin id			 */
		vi_cb,			/* Name of callback function	 */
		"vi:  Runs editor, result goes to chat",
		NULL			/* User-defined data		 */
	);
	/*								 */
	cat_command_id = purple_cmd_register(
		"cat",			/* Command name			 */
		"s",			/* Command arg format		 */
		PURPLE_CMD_P_DEFAULT,	/* Command priority		 */
		(PURPLE_CMD_FLAG_IM|PURPLE_CMD_FLAG_CHAT),
		me,			/* Plugin id			 */
		cat_cb,			/* Name of callback function	 */
		"cat &lt;filename&gt;:  Copy file slowly to chat",
		NULL			/* User-defined data		 */
	);
	return( TRUE );
}

static	gboolean
plugin_unload(
	PurplePlugin *		plugin
)
{
	purple_cmd_unregister( vim_command_id );
	purple_cmd_unregister( cat_command_id );
	return( TRUE );
}

#if	0
static void
action_test(
	PurplePluginAction *	action
)
{
	purple_notify_message(
		plugin_id,
		PURPLE_NOTIFY_MSG_INFO,
		"Plugin Actions Test",
		"This is a plugin actions test :)",
		NULL,
		NULL,
		NULL
	);
}

static	GList *
do_actions(
	PurplePlugin *		plugin,
	gpointer		context
)
{
	GList *			retval;

	retval = NULL;
	do	{
		PurplePluginAction *	action = purple_plugin_action_new(
			"Plugin Action Test",
			action_test
		);
		retval = g_list_append( retval, action );
	} while( 0 );
	return( retval );
}
#endif	/* NOPE */

static PurplePluginInfo info =	{
	PURPLE_PLUGIN_MAGIC,		/* Magic number			 */
	PURPLE_MAJOR_VERSION,		/* Version we compiled against	 */
	PURPLE_MINOR_VERSION,		/* Version we compiled against	 */
	PURPLE_PLUGIN_STANDARD,		/* Just yer standard plugin	 */
	NULL,				/* Normal type			 */
	0,				/* Plugin flags			 */
	NULL,				/* GList of plugin dependancies  */
	PURPLE_PRIORITY_DEFAULT,	/* Humdrum priority		 */
	(char *) me,			/* Plugin's ID			 */
	"cat amongst the pidgins",	/* Plugin's name		 */
	"1.1",				/* Plugin's version		 */
	"Run editor/cat file, post results",	/* Summary		 */
	"You should not really need further description.", /* Description */
	"Tommy Reynolds <Tommy.Reynolds@MegaCoder.com>", /* Owner	 */
	"http://www.MegaCoder.com",	/* URL				 */
	plugin_load,			/* Called when module loaded	 */
	plugin_unload,			/* Called before module unloaded */
	NULL,				/* Called before destroying	 */
	NULL,				/* GUI-specific info		 */
	NULL,				/* Reserved			 */
	NULL,				/* Reserved			 */
#if	1
	NULL,
#else	/* NOPE */
	do_actions,			/* Action callback		 */
#endif	/* NOPE */
	NULL,				/* Reserved			 */
	NULL,				/* Reserved			 */
	NULL,				/* Reserved			 */
	NULL				/* Reserved			 */
};

static void
init_plugin(
	PurplePlugin *		plugin
)
{
	/* I'm getting nuttin' for Christmas				 */
}

PURPLE_INIT_PLUGIN( me , init_plugin, info )
