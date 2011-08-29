#ifndef ragel_declare_h
#define ragel_declare_h

/*
 * Utu -- Saving The Internet With Hate
 *
 * Copyright (c) Zed A. Shaw 2005 (zedshaw@zedshaw.com)
 *
 * This file is modifiable/redistributable under the terms of the GNU
 * General Public License.
 *
 * You should have recieved a copy of the GNU General Public License along
 * with this program; see the file COPYING. If not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 0211-1307, USA.
 */


#define RAGEL_DECLARE_FUNCTIONS(name) int name##_init(name *parser);\
size_t name##_execute(name *parser, const char *buffer, size_t len, size_t off);\
int name##_has_error(name *parser);\
int name##_is_finished(name *parser);\
int name##_finish(name *parser);


#endif
