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
#include "iscsi.h"
#include "scsi-lowlevel.h"
#include "iscsi-test.h"

int T0101_read10_beyond_eol(const char *initiator, const char *url)
{ 
	struct iscsi_context *iscsi;
	struct scsi_task *task;
	struct scsi_readcapacity10 *rc10;
	int ret, i, lun;
	uint32_t block_size, num_blocks;

	iscsi = iscsi_context_login(initiator, url, &lun);
	if (iscsi == NULL) {
		printf("Failed to login to target\n");
		return -1;
	}

	/* find the size of the LUN */
	task = iscsi_readcapacity10_sync(iscsi, lun, 0, 0);
	if (task == NULL) {
		printf("Failed to send readcapacity10 command: %s\n", iscsi_get_error(iscsi));
		ret = -1;
		goto finished;
	}
	if (task->status != SCSI_STATUS_GOOD) {
		printf("Readcapacity command: failed with sense. %s\n", iscsi_get_error(iscsi));
		ret = -1;
		scsi_free_scsi_task(task);
		goto finished;
	}
	rc10 = scsi_datain_unmarshall(task);
	if (rc10 == NULL) {
		printf("failed to unmarshall readcapacity10 data. %s\n", iscsi_get_error(iscsi));
		ret = -1;
		scsi_free_scsi_task(task);
		goto finished;
	}
	block_size = rc10->block_size;
	num_blocks = rc10->lba;
	scsi_free_scsi_task(task);



	ret = 0;

	/* read 1-256 blocks, one block beyond the end-of-lun */
	printf("Reading last 1-256 blocks one block beyond eol ... ");
	for (i=1; i<=256; i++) {
		task = iscsi_read10_sync(iscsi, lun, num_blocks + 2 - i, i * block_size, block_size);
		if (task == NULL) {
			printf("[FAILED]\n");
			printf("Failed to send read10 command: %s\n", iscsi_get_error(iscsi));
			ret = -1;
			goto finished;
		}
		if (task->status == SCSI_STATUS_GOOD) {
			printf("[FAILED]\n");
			printf("Read10 beyond end-of-lun did not fail with sense.\n");
			ret = -1;
			scsi_free_scsi_task(task);
			goto finished;
		}
		if (task->sense.key != SCSI_SENSE_ILLEGAL_REQUEST) {
			printf("[FAILED]\n");
			printf("Read10 beyond end-of-lun did not return sense key ILLEGAL_REQUEST.\n");
			ret = -1;
			scsi_free_scsi_task(task);
			goto finished;
		}
		if (task->sense.ascq != SCSI_SENSE_ASCQ_LBA_OUT_OF_RANGE) {
			printf("[FAILED]\n");
			printf("Read10 beyond end-of-lun did not return sense ascq LBA OUT OF RANGE.\n");
			ret = -1;
			scsi_free_scsi_task(task);
			goto finished;
		}
		scsi_free_scsi_task(task);
	}
	printf("[OK]\n");


	/* read 2-256 blocks, all but one block beyond the eol */
	printf("Reading 1-255 blocks beyond eol starting at last block ... ");
	for (i=2; i<=256; i++) {
		task = iscsi_read10_sync(iscsi, lun, num_blocks, i * block_size, block_size);
		if (task == NULL) {
			printf("[FAILED]\n");
			printf("Failed to send read10 command: %s\n", iscsi_get_error(iscsi));
			ret = -1;
			goto finished;
		}
		if (task->status == SCSI_STATUS_GOOD) {
			printf("[FAILED]\n");
			printf("Read10 beyond end-of-lun did not return sense.\n");
			ret = -1;
			scsi_free_scsi_task(task);
			goto finished;
		}
		if (task->sense.key != SCSI_SENSE_ILLEGAL_REQUEST) {
			printf("[FAILED]\n");
			printf("Read10 beyond end-of-lun did not return sense key ILLEGAL_REQUEST.\n");
			ret = -1;
			scsi_free_scsi_task(task);
			goto finished;
		}
		if (task->sense.ascq != SCSI_SENSE_ASCQ_LBA_OUT_OF_RANGE) {
			printf("[FAILED]\n");
			printf("Read10 beyond end-of-lun did not return sense ascq LBA OUT OF RANGE.\n");
			ret = -1;
			scsi_free_scsi_task(task);
			goto finished;
		}
		scsi_free_scsi_task(task);
	}
	printf("[OK]\n");


finished:
	iscsi_logout_sync(iscsi);
	iscsi_destroy_context(iscsi);
	return ret;
}
