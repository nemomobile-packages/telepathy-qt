/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009 Nokia Corporation
 * @license LGPL 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _TelepathyQt_global_h_HEADER_GUARD_
#define _TelepathyQt_global_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <QtGlobal>

#ifdef BUILDING_TP_QT
#  define TP_QT_EXPORT Q_DECL_EXPORT
#else
#  define TP_QT_EXPORT Q_DECL_IMPORT
#endif

#if !defined(Q_OS_WIN) && defined(QT_VISIBILITY_AVAILABLE)
#  define TP_QT_NO_EXPORT __attribute__((visibility("hidden")))
#endif

#ifndef TP_QT_NO_EXPORT
#  define TP_QT_NO_EXPORT
#endif

/**
 * @def TP_QT_DEPRECATED
 * @ingroup macros
 *
 * The TP_QT_DEPRECATED macro can be used to trigger compile-time
 * warnings with newer compilers when deprecated functions are used.
 *
 * For non-inline functions, the macro gets inserted at front of the
 * function declaration, right before the return type:
 *
 * \code
 * TP_QT_DEPRECATED void deprecatedFunctionA();
 * TP_QT_DEPRECATED int deprecatedFunctionB() const;
 * \endcode
 *
 * For functions which are implemented inline,
 * the TP_QT_DEPRECATED macro is inserted at the front, right before the
 * return type, but after "static", "inline" or "virtual":
 *
 * \code
 * TP_QT_DEPRECATED void deprecatedInlineFunctionA() { .. }
 * virtual TP_QT_DEPRECATED int deprecatedInlineFunctionB() { .. }
 * static TP_QT_DEPRECATED bool deprecatedInlineFunctionC() { .. }
 * inline TP_QT_DEPRECATED bool deprecatedInlineFunctionD() { .. }
 * \endcode
 *
 * You can also mark whole structs or classes as deprecated, by inserting the
 * TP_QT_DEPRECATED macro after the struct/class keyword, but before the
 * name of the struct/class:
 *
 * \code
 * class TP_QT_DEPRECATED DeprecatedClass { };
 * struct TP_QT_DEPRECATED DeprecatedStruct { };
 * \endcode
 *
 * \note If the class you want to deprecate is a QObject and needs to be exported,
 *       you should use TP_QT_EXPORT_DEPRECATED instead.
 *
 * \note
 * It does not make much sense to use the TP_QT_DEPRECATED keyword for a
 * Qt signal; this is because usually get called by the class which they belong
 * to, and one would assume that a class author does not use deprecated methods
 * of his own class. The only exception to this are signals which are connected
 * to other signals; they get invoked from moc-generated code. In any case,
 * printing a warning message in either case is not useful.
 * For slots, it can make sense (since slots can be invoked directly) but be
 * aware that if the slots get triggered by a signal, they will get called from
 * moc code as well and thus the warnings are useless.
 *
 * \note
 * TP_QT_DEPRECATED cannot be used for constructors.
 */
#ifndef TP_QT_DEPRECATED
#  ifdef TP_QT_DEPRECATED_WARNINGS
#    ifdef BUILDING_TP_QT
#      define TP_QT_DEPRECATED
#    else
#      define TP_QT_DEPRECATED Q_DECL_DEPRECATED
#    endif
#  else
#    define TP_QT_DEPRECATED
#  endif
#endif

/**
 * @def TP_QT_EXPORT_DEPRECATED
 * @ingroup macros
 *
 * The TP_QT_EXPORT_DEPRECATED macro can be used to trigger compile-time
 * warnings with newer compilers when deprecated functions are used, and
 * export the symbol.
 *
 * This macro simply expands to TP_QT_DEPRECATED TP_QT_EXPORT, and needs
 * to be used only when you need to deprecate a class which is a QObject
 * and needs to be exported. This is because the following:
 *
 * \code
 * class TP_QT_DEPRECATED TP_QT_EXPORT Class : public QObject
 * \endcode
 *
 * Wouldn't be recognized from moc to be a valid QObject class, and hence
 * would be skipped. Instead, you should do:
 *
 * \code
 * class TP_QT_EXPORT_DEPRECATED Class : public QObject
 * \endcode
 *
 * For any other use, please use TP_QT_DEPRECATED instead.
 */
#ifndef TP_QT_EXPORT_DEPRECATED
#  define TP_QT_EXPORT_DEPRECATED TP_QT_DEPRECATED TP_QT_EXPORT
#endif

#endif
