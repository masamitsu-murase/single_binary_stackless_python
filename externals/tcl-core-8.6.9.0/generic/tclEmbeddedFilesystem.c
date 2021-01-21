/*
 * tclTest.c --
 *
 *	This file contains C command functions for a bunch of additional Tcl
 *	commands that are used for testing out Tcl's C interfaces. These
 *	commands are not normally included in Tcl applications; they're only
 *	used for testing.
 *
 * Copyright (c) 1993-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-2000 Ajuba Solutions.
 * Copyright (c) 2003 by Kevin B. Kenny.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include "tclWinInt.h"
#include "tclOO.h"
#include <math.h>

/*
 * Required for TestlocaleCmd
 */
#include <locale.h>

/*
 * Required for the TestChannelCmd and TestChannelEventCmd
 */
#include "tclIO.h"

#include "tclEmbeddedFilesystem.h"

#define PREFIX_LEN 12

typedef struct tag_EmbeddedFileChannelInfo
{
    EmbeddedFileInfo *info;
    Tcl_Channel channel;
    int offset;
} EmbeddedFileChannelInfo;

static const Tcl_ChannelType embeddedChannelType;

static int
EmbeddedClose(
    ClientData instanceData,
    Tcl_Interp *interp)
{
    ckfree(instanceData);
    return TCL_OK;
}

static int
EmbeddedInput(
    ClientData instanceData,
    char *buf,
    int toRead,
    int *errorCodePtr)
{
    EmbeddedFileChannelInfo *channelInfo = instanceData;
    *errorCodePtr = 0;
    if (channelInfo->offset + toRead <= channelInfo->info->file_size) {
        memcpy(buf, &channelInfo->info->file_content[channelInfo->offset], toRead);
        channelInfo->offset += toRead;
        return toRead;
    } else if (channelInfo->offset < channelInfo->info->file_size) {
        int restSize = channelInfo->info->file_size - channelInfo->offset;
        memcpy(buf, &channelInfo->info->file_content[channelInfo->offset], restSize);
        channelInfo->offset = channelInfo->info->file_size;
        return restSize;
    } else {
        return 0;
    }
}

static int
EmbeddedOutput(
    ClientData instanceData,
    const char *buf,
    int toWrite,
    int *errorCodePtr)
{
    Tcl_SetErrno(EACCES);
    return -1;
}

static void
EmbeddedWatch(
    ClientData instanceData,
    int mask)
{
    Tcl_SetErrno(EINVAL);
}

static int
EmbeddedGetHandle(
    ClientData instanceData,
    int direction,
    ClientData *handlePtr)
{
    Tcl_SetErrno(EINVAL);
    return -1;
}

static const Tcl_ChannelType embeddedChannelType = {
    "embedded",			/* Type name. */
    TCL_CHANNEL_VERSION_2,
    EmbeddedClose,
    EmbeddedInput,
    EmbeddedOutput,
    NULL,			/* seekProc */
    NULL,
    NULL,
    EmbeddedWatch,
    EmbeddedGetHandle,
    NULL,			/* close2Proc */
    NULL,
    NULL,			/* flushProc */
    NULL,
    NULL,			/* wideSeekProc */
    NULL,
    NULL
};

static const Tcl_Filesystem embeddedFilesystem;

static EmbeddedFileInfo *
FindFileInfo(const char *name)
{
	int i;
	for (i = 0; i < gEmbeddedFileInfoCount; i++) {
		if (strcmp(name + PREFIX_LEN, gEmbeddedFileInfo[i].name) == 0) {
			return &gEmbeddedFileInfo[i];
		}
	}
	return NULL;
}

static int
EmbeddedPathInFilesystem(
    Tcl_Obj *pathPtr,
    ClientData *clientDataPtr)
{
    const char *str = Tcl_GetString(pathPtr);

    if (strncmp(str, "embeddedfs:/", PREFIX_LEN)) {
	return -1;
    }
    return TCL_OK;
}

static Tcl_Channel
EmbeddedOpenFileChannel(
    Tcl_Interp *interp,		/* Interpreter for error reporting; can be
				 * NULL. */
    Tcl_Obj *pathPtr,		/* Name of file to open. */
    int mode,			/* POSIX open mode. */
    int permissions)		/* If the open involves creating a file, with
				 * what modes to create it? */
{
    const char *str = Tcl_GetString(pathPtr);
	EmbeddedFileInfo *info = FindFileInfo(str);
    EmbeddedFileChannelInfo *infoPtr;
    char channelName[64];

    if ((mode != 0) && !(mode & O_RDONLY)) {
	Tcl_AppendResult(interp, "read-only", NULL);
	return NULL;
    }

    if (info == NULL) {
        Tcl_AppendResult(interp, "not-found", NULL);
        return NULL;
    } else if (info->type != EMBEDDED_FILE_INFO_TYPE_FILE) {
        Tcl_AppendResult(interp, "cannot-open", NULL);
        return NULL;
    }

    infoPtr = ckalloc(sizeof(EmbeddedFileChannelInfo));
    infoPtr->info = info;
    infoPtr->offset = 0;
    sprintf(channelName, "embeddedfs:%" TCL_I_MODIFIER "x", (size_t) infoPtr);

    infoPtr->channel = Tcl_CreateChannel(&embeddedChannelType, channelName, infoPtr,
                                         TCL_READABLE);
    return infoPtr->channel;
}

static int
EmbeddedAccess(
    Tcl_Obj *pathPtr,		/* Path of file to access (in current CP). */
    int mode)			/* Permission setting. */
{
    const char *str = Tcl_GetString(pathPtr);
	EmbeddedFileInfo *info = FindFileInfo(str);
	if (info == NULL) {
	    Tcl_SetErrno(ENOENT);
		return -1;
	}

	switch (mode) {
		case 0x00:
		case 0x04:
		    return 0;
		case 0x02:
		case 0x06:
			Tcl_SetErrno(EACCES);
			return -1;
		default:
			Tcl_SetErrno(EINVAL);
			return -1;
	}
}

static int
EmbeddedStat(
    Tcl_Obj *pathPtr,		/* Path of file to stat (in current CP). */
    Tcl_StatBuf *bufPtr)	/* Filled with results of stat call. */
{
    Tcl_StatBuf empty_buf = {0};
    const char *str = Tcl_GetString(pathPtr);
	EmbeddedFileInfo *info = FindFileInfo(str);
	if (info == NULL) {
	    Tcl_SetErrno(ENOENT);
		return -1;
	}

    *bufPtr = empty_buf;
	bufPtr->st_size = info->file_size;
    switch (info->type) {
        case EMBEDDED_FILE_INFO_TYPE_DIRECTORY:
            bufPtr->st_mode = S_IFDIR;
            break;
        case EMBEDDED_FILE_INFO_TYPE_FILE:
        default:
            bufPtr->st_mode = S_IFREG;
            break;
    }
    return 0;
}

static Tcl_Obj *
EmbeddedListVolumes(void)
{
    /* Add one new volume */
    Tcl_Obj *retVal;

    retVal = Tcl_NewStringObj("embeddedfs:/", -1);
    Tcl_IncrRefCount(retVal);
    return retVal;
}

static const Tcl_Filesystem embeddedFilesystem = {
    "embedded",
    sizeof(Tcl_Filesystem),
    TCL_FILESYSTEM_VERSION_1,
    EmbeddedPathInFilesystem,
    NULL,
    NULL,
    /* No internal to normalized, since we don't create any
     * pure 'internal' Tcl_Obj path representations */
    NULL,
    /* No create native rep function, since we don't use it
     * or 'Tcl_FSNewNativePath' */
    NULL,
    /* Normalize path isn't needed - we assume paths only have
     * one representation */
    NULL,
    NULL,
    NULL,
    EmbeddedStat,
    EmbeddedAccess,
    EmbeddedOpenFileChannel,
    NULL,
    NULL,
    /* We choose not to support symbolic links inside our vfs's */
    NULL,
    EmbeddedListVolumes,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    /* No copy file - fallback will occur at Tcl level */
    NULL,
    /* No rename file - fallback will occur at Tcl level */
    NULL,
    /* No copy directory - fallback will occur at Tcl level */
    NULL,
    /* Use stat for lstat */
    NULL,
    /* No load - fallback on core implementation */
    NULL,
    /* We don't need a getcwd or chdir - fallback on Tcl's versions */
    NULL,
    NULL
};

void TclEmbeddedFilesystemRegister()
{
    EmbeddedFileInfoDataInitialize();
    Tcl_FSRegister(NULL, &embeddedFilesystem);
}
