/* keyimport.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
 *
 * This file is part of wolfTPM.
 *
 * wolfTPM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfTPM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* Tool and example for creating, storing and loading keys using TPM2.0 */

#include <wolftpm/tpm2_wrap.h>

#include <examples/keygen/keygen.h>
#include <examples/tpm_io.h>
#include <examples/tpm_test.h>

#include <stdio.h>


/******************************************************************************/
/* --- BEGIN TPM Key Import / Blob Example -- */
/******************************************************************************/

static void usage(void)
{
    printf("Expected usage:\n");
    printf("keyimport [keyblob.bin] [ECC/RSA]\n");
}


int TPM2_Keyimport_Example(void* userCtx, int argc, char *argv[])
{
    int rc;
    WOLFTPM2_DEV dev;
    WOLFTPM2_KEY storage; /* SRK */
    WOLFTPM2_KEYBLOB impKey;
    TPMS_AUTH_COMMAND session[MAX_SESSION_NUM];
    TPMI_ALG_PUBLIC alg = TPM_ALG_RSA; /* TPM_ALG_ECC */

#if !defined(WOLFTPM2_NO_WOLFCRYPT) && !defined(NO_FILESYSTEM)
    XFILE f;
    const char* outputFile = "keyblob.bin";
    size_t fileSz = 0;

    if (argc >= 2) {
        if (XSTRNCMP(argv[1], "-?", 2) == 0 || 
            XSTRNCMP(argv[1], "--help", 6) == 0) {
            usage();
            return 0;
        }

        outputFile = argv[1];
        if (argc >= 3) {
            /* ECC vs RSA */
            if (XSTRNCMP(argv[2], "ECC", 3) == 0) {
                alg = TPM_ALG_ECC;
            }
        }
    }
#else
    (void)argc;
    (void)argv;
#endif
    XMEMSET(session, 0, sizeof(session));
    XMEMSET(&storage, 0, sizeof(storage));

    printf("TPM2.0 Key Import example\n");
    rc = wolfTPM2_Init(&dev, TPM2_IoCb, userCtx);
    if (rc != TPM_RC_SUCCESS) {
        printf("\nwolfTPM2_Init failed\n");
        goto exit;
    }

    /* Define the default session auth that has NULL password */
    session[0].sessionHandle = TPM_RS_PW;
    session[0].auth.size = 0;
    TPM2_SetSessionAuth(session);

    /* See if SRK already exists */
    rc = wolfTPM2_ReadPublicKey(&dev, &storage, TPM2_DEMO_STORAGE_KEY_HANDLE);
    if (rc != 0) {
        printf("Loading SRK: Storage failed 0x%x: %s\n", rc,
            TPM2_GetRCString(rc));
        goto exit;
    }
    printf("Loading SRK: Storage 0x%x (%d bytes)\n",
        (word32)storage.handle.hndl, storage.pub.size);

    /* set session for authorization of the storage key */
    session[0].auth.size = sizeof(gStorageKeyAuth)-1;
    XMEMCPY(session[0].auth.buffer, gStorageKeyAuth, session[0].auth.size);

    XMEMSET(&impKey, 0, sizeof(impKey));
    if (alg == TPM_ALG_RSA) {
        /* Import raw RSA private key into TPM */
        rc = wolfTPM2_ImportRsaPrivateKey(&dev, &storage, &impKey,
            kRsaKeyPubModulus, (word32)sizeof(kRsaKeyPubModulus),
            kRsaKeyPubExponent,
            kRsaKeyPrivQ,      (word32)sizeof(kRsaKeyPrivQ), 
            TPM_ALG_NULL, TPM_ALG_NULL);
    }
    else if (alg == TPM_ALG_ECC) {
        /* Import raw ECC private key into TPM */
        rc = wolfTPM2_ImportEccPrivateKey(&dev, &storage, &impKey, 
            TPM_ECC_NIST_P256,
            kEccKeyPubXRaw, (word32)sizeof(kEccKeyPubXRaw),
            kEccKeyPubYRaw, (word32)sizeof(kEccKeyPubYRaw),
            kEccKeyPrivD,   (word32)sizeof(kEccKeyPrivD));
    }
    if (rc != 0) goto exit;

    printf("Imported key (pub %d, priv %d bytes)\n",
        impKey.pub.size, impKey.priv.size);

    /* Save key as encrypted blob to the disk */
#if !defined(WOLFTPM2_NO_WOLFCRYPT) && !defined(NO_FILESYSTEM)
    f = XFOPEN(outputFile, "wb");
    if (f != XBADFILE) {
        impKey.pub.size = sizeof(impKey.pub);
        fileSz += XFWRITE(&impKey.pub, 1, sizeof(impKey.pub), f);
        fileSz += XFWRITE(&impKey.priv, 1, sizeof(UINT16) + impKey.priv.size, f);
        XFCLOSE(f);
    }
    printf("Wrote %d bytes to %s\n", (int)fileSz, outputFile);
#else
    printf("Key Public Blob %d\n", impKey.pub.size);
    TPM2_PrintBin((const byte*)&impKey.pub.publicArea, impKey.pub.size);
    printf("Key Private Blob %d\n", impKey.priv.size);
    TPM2_PrintBin(impKey.priv.buffer, impKey.priv.size);
#endif

exit:

    if (rc != 0) {
        printf("\nFailure 0x%x: %s\n\n", rc, wolfTPM2_GetRCString(rc));
    }

    /* Close key handles */
    wolfTPM2_UnloadHandle(&dev, &impKey.handle);

    wolfTPM2_Cleanup(&dev);
    return rc;
}

/******************************************************************************/
/* --- END TPM Timestamp Test -- */
/******************************************************************************/


#ifndef NO_MAIN_DRIVER
int main(int argc, char *argv[])
{
    int rc;

    rc = TPM2_Keyimport_Example(NULL, argc, argv);

    return rc;
}
#endif
