/***********************************************************************
 *
 *  Copyright (c) 2018  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2018:proprietary:standard

 This program is the proprietary software of Broadcom and/or its
 licensors, and may only be used, duplicated, modified or distributed pursuant
 to the terms and conditions of a separate, written license agreement executed
 between you and Broadcom (an "Authorized License").  Except as set forth in
 an Authorized License, Broadcom grants no license (express or implied), right
 to use, or waiver of any kind with respect to the Software, and Broadcom
 expressly reserves all rights in and to the Software and all intellectual
 property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.

 Except as expressly set forth in the Authorized License,

 1. This program, including its structure, sequence and organization,
    constitutes the valuable trade secrets of Broadcom, and you shall use
    all reasonable efforts to protect the confidentiality thereof, and to
    use this information only in connection with your use of Broadcom
    integrated circuit products.

 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
    AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
    WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
    RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
    ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
    FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
    COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
    TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
    PERFORMANCE OF THE SOFTWARE.

 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
    ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
    INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
    WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
    IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
    OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
    SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
    SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
    LIMITED REMEDY.
:>
 *
 ************************************************************************/

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "os_defs.h"
#include "nvc.h"

int main()
{
    char *list_a = NULL;
    char *list_b = NULL;
    char *p;

    list_b = nvc_list_update(list_a, "chestnut", "jason");

    list_a = nvc_list_update(list_b, "eagle", "bird");
    free(list_b);

    list_b = nvc_list_update(list_a, "rice", "pudding");
    free(list_a);

    list_a = nvc_list_update(list_b, "chocolate", "black");
    free(list_b);

    printf("After append: %s\n", list_a);

    /* Test search */
    p = nvc_list_get(list_a, "rice");
    if (0 != strcmp(p, "pudding"))
    {
        printf("Test Failed: Searching returned unexpecetd string: %s\n", p);
        free(p);
        exit(0);
    }
    else
    {
        printf("Searching for \"rice\" returned expecetd string: %s\n", p);
    }
    free(p);

    /* Test replace */
    list_b = nvc_list_update(list_a, "eagle", "feather");
    free(list_a);
    if (list_b == NULL)
    {
        printf("Test Failed: Failed to replace \"eagle\"\n");
        exit(0);
    }
    else
    {
        printf("After replacing: %s\n", list_b);
    }
    p = nvc_list_get(list_b, "eagle");
    if (0 != strcmp(p, "feather"))
    {
        printf("Test Failed: Searching returned unexpecetd string: %s\n", p);
        free(p);
        exit(0);
    }
    free(p);

    /* Test delete */
    list_a = nvc_list_delete(list_b, "eagle");
    free(list_b);
    if (list_a != NULL)
    {
        printf("After deleting \"eagle\": %s\n", list_a);
        free(list_a);
    }
    else
    {
        printf("Test Failed: Deleting operation failed.");
        exit(0);
    }

    printf("Test Passed!\n");
}

