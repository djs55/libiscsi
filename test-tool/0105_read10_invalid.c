/* 
   Copyright (C) 2010 by Ronnie Sahlberg <ronniesahlberg@gmail.com>
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iscsi.h"
#include "scsi-lowlevel.h"
#include "iscsi-test.h"

int T0105_read10_invalid(const char *initiator, const char *url)
{ 
	struct iscsi_context *iscsi;
	struct scsi_task *task;
	struct iscsi_data data;
	char buf[512];
	int ret, lun;

	iscsi = iscsi_context_login(initiator, url, &lun);
	if (iscsi == NULL) {
		printf("Failed to login to target\n");
		return -1;
	}


	ret = 0;

	/* Try a read of 1 block but xferlength == 0 */
	printf("Read10 1 block but with iscsi ExpectedDataTransferLength==0 ... ");

	task = malloc(sizeof(struct scsi_task));
	if (task == NULL) {
		printf("Failed to allocate task structure\n");
		ret = -1;
		goto finished;
	}

	memset(task, 0, sizeof(struct scsi_task));
	task->cdb[0] = SCSI_OPCODE_READ10;
	task->cdb[8] = 1;
	task->cdb_size = 10;
	task->xfer_dir = SCSI_XFER_READ;
	task->expxferlen = 0;

	if (iscsi_scsi_command_sync(iscsi, lun, task, NULL) == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send read10 command: %s\n", iscsi_get_error(iscsi));
		ret = -1;

		goto finished;
	}
	if (task->status == SCSI_STATUS_GOOD) {
	        printf("[FAILED]\n");
		printf("Read10 of 1 block with iscsi ExpectedDataTransferLength==0 should fail.\n");
		ret = -1;
		scsi_free_scsi_task(task);
		goto finished;
	}
	scsi_free_scsi_task(task);
	printf("[OK]\n");


	/* Try a read of 1 block but make it a data-out write on the iscsi layer */
	printf("Read10 of 1 block but sent as data-out write in iscsi layer ... ");

	task = malloc(sizeof(struct scsi_task));
	if (task == NULL) {
		printf("Failed to allocate task structure\n");
		ret = -1;
		goto finished;
	}

	memset(task, 0, sizeof(struct scsi_task));
	task->cdb[0] = SCSI_OPCODE_READ10;
	task->cdb[8] = 1;
	task->cdb_size = 10;
	task->xfer_dir = SCSI_XFER_WRITE;
	task->expxferlen = sizeof(buf);

	data.size = sizeof(buf);
	data.data = &buf[0];

	if (iscsi_scsi_command_sync(iscsi, lun, task, &data) == NULL) {
	        printf("[FAILED]\n");
		printf("Failed to send read10 command: %s\n", iscsi_get_error(iscsi));
		ret = -1;

		goto finished;
	}
	if (task->status == SCSI_STATUS_GOOD) {
	        printf("[FAILED]\n");
		printf("Read10 of 1 block but iscsi data-out write should fail.\n");
		ret = -1;
		scsi_free_scsi_task(task);
		goto finished;
	}
	scsi_free_scsi_task(task);
	printf("[OK]\n");


finished:
	iscsi_logout_sync(iscsi);
	iscsi_destroy_context(iscsi);
	return ret;
}
