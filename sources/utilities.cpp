// This file is part of BellePoule.
//
//   BellePoule is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   BellePoule is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with BellePoule.  If not, see <http://www.gnu.org/licenses/>.

#include <gtk/gtk.h>
#include "utilities.hpp"

/**
 * ScanDataDirs
 * @path_segment: The portion of the path to scan for relative to various data directories
 * @test: The type of test to perform while scanning
 *
 * Creates a filename or directory name for a path segment that resides in one
 * of the standard data directories.  The file is first searched for in the user
 * data directory, then in each of the system data directories and finally in
 * the local source tree.
 *
 * The returned path is the location of the path in the prioritized path list
 *
 * Return value: a newly-allocated string that must be freed with g_free().
 */
gchar * ScanDataDirs (const gchar *path_segment, GFileTest test)
{
	gchar* test_path = NULL; // The returned filename
	const gchar * const *xdg_data_dirs;  // The system data directories

	// Try the path segment in the user data directory
	test_path = g_build_filename (g_get_user_data_dir (), "BellePoule",
									path_segment, NULL);
	if (!g_file_test(test_path, test))
	{
		// The path was not in the user data dir.  Clean up and look in the
		// system dirs
		g_free(test_path);
		test_path=NULL;
		xdg_data_dirs = g_get_system_data_dirs ();
		for (int i = 0; xdg_data_dirs[i]; i++)
		{
			// Try the system dirs one at a time
			test_path = g_build_filename(xdg_data_dirs[i], "BellePoule",
											path_segment, NULL);
			if (!g_file_test(test_path, test))
			{
				// Not in this one.  Clean up and try again.
				g_free(test_path);
				test_path=NULL;
			}
			else
			{
				// Found it!
				break;
			}
		}
		if (!test_path )
		{
			// Try in the source tree (really relative to the
			// program build location)
			gchar *program_dir = g_get_current_dir ();
			if (program_dir)
			{
				test_path = g_build_filename(program_dir,"..","..","resources",
														path_segment, NULL);
				if (!g_file_test(test_path, test))
				{
					// Not found.  Try a subdirectory of the current directory
					g_free(test_path);
					test_path=NULL;
					test_path = g_build_filename(program_dir,"resources",
													path_segment, NULL);
					if (!g_file_test(test_path, test))
					{
						// Still not found.  Clean up.
						g_free(test_path);
						test_path = NULL;
					}
				}
				g_free(program_dir);
			}
		}
	}
	return test_path;
}

// --------------------------------------------------------------------------------
/**
 * FindDataFile
 * @first_element: the first element in the path relative to a data directory
 * @Varargs: remaining elements in the relative path, terminated by %NULL
 *
 * Creates a filename for a file that resides in one of the standard data
 * directories.  The file is first searched for in the user data directory, the
 * in each of the system data directories and finally in the local source tree.
 *
 * Return value: a newly-allocated string that must be freed with g_free().
 */
gchar * FindDataFile (const gchar *first_element, ...)
{
	gchar* data_file = NULL; // The returned filename
	gchar* path_segment = NULL; // The parts of the path supplied by the caller

	// Build the part of the filename passed in as arguments
	gchar **args;
	gchar *s=NULL;
	va_list va_args;
	int i=0,count=0;

	// Count the number of variable arguments
	va_start (va_args, first_element);
	s = va_arg(va_args, gchar* );
	for(count=0; s; count++)
	{
		s = va_arg(va_args, gchar* );
	}

	count+=2; // 'count' also includes the first element and the terminal 'NULL'
	args = g_new (gchar*, count);

	// Assign the variable arguments to the argument vector
	va_start (va_args, first_element);
	args[0] = g_strdup(first_element);
	for(i=1; i<count; i++)
	{
		args[i] = va_arg(va_args, gchar* );
	}
	va_end (va_args);

	path_segment = g_build_filenamev(args);

	// Scan through the data directories for the file
	data_file = ScanDataDirs(path_segment, G_FILE_TEST_IS_REGULAR);

	g_free(path_segment);
	return data_file;
}

// --------------------------------------------------------------------------------
/**
 * FindDataDir
 * @first_element: the first element in the path relative to a data directory
 * @Varargs: remaining elements in the relative path, terminated by %NULL
 *
 * Creates a directory name for a subdirectory that resides in one of the
 * standard data directories.  The directory is first searched for in the user
 * data directory, then in each of the system data directories and finally in
 * the local source tree.
 *
 * Return value: a newly-allocated string that must be freed with g_free().
 */
gchar * FindDataDir (const gchar *first_element, ...)
{
	gchar* data_dir = NULL; // The returned filename
	gchar* path_segment = NULL; // The parts of the path supplied by the caller

	// Build the part of the filename passed in as arguments
	gchar **args;
	gchar *s=NULL;
	va_list va_args;
	int i=0,count=0;

	// Count the number of variable arguments
	va_start (va_args, first_element);
	s = va_arg(va_args, gchar* );
	for(count=0; s; count++)
	{
		s = va_arg(va_args, gchar* );
	}

	count+=2; // 'count' also includes the first element and the terminal 'NULL'
	args = g_new (gchar*, count);

	// Assign the variable arguments to the argument vector
	va_start (va_args, first_element);
	args[0] = g_strdup(first_element);
	for(i=1; i<count; i++)
	{
		args[i] = va_arg(va_args, gchar* );
	}
	va_end (va_args);

	path_segment = g_build_filenamev(args);

	// Scan through the data directories for the sub directory
	data_dir = ScanDataDirs(path_segment, G_FILE_TEST_IS_DIR);

	g_free(path_segment);
	return data_dir;
}
