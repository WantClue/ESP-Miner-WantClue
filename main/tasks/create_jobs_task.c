#include "work_queue.h"
#include "global_state.h"
#include "esp_log.h"
#include "esp_system.h"
#include "mining.h"
#include <limits.h>
#include "string.h"

#include <sys/time.h>

static const char *TAG = "create_jobs_task";
int test_mode = 0;

void create_jobs_task(void *pvParameters)
{

    GlobalState *GLOBAL_STATE = (GlobalState *)pvParameters;

    while (1)
    {
        if(!test_mode) {
        mining_notify *mining_notification = (mining_notify *)queue_dequeue(&GLOBAL_STATE->stratum_queue);
        ESP_LOGI(TAG, "New Work Dequeued %s", mining_notification->job_id);

        uint32_t extranonce_2 = 0;
        while (GLOBAL_STATE->stratum_queue.count < 1 && extranonce_2 < UINT_MAX && GLOBAL_STATE->abandon_work == 0)
        {
            char *extranonce_2_str = extranonce_2_generate(extranonce_2, GLOBAL_STATE->extranonce_2_len);

            char *coinbase_tx = construct_coinbase_tx(mining_notification->coinbase_1, mining_notification->coinbase_2, GLOBAL_STATE->extranonce_str, extranonce_2_str);

            char *merkle_root = calculate_merkle_root_hash(coinbase_tx, (uint8_t(*)[32])mining_notification->merkle_branches, mining_notification->n_merkle_branches);
            bm_job next_job = construct_bm_job(mining_notification, merkle_root, GLOBAL_STATE->version_mask);

            bm_job *queued_next_job = malloc(sizeof(bm_job));
            memcpy(queued_next_job, &next_job, sizeof(bm_job));
            queued_next_job->extranonce2 = strdup(extranonce_2_str);
            queued_next_job->jobid = strdup(mining_notification->job_id);
            queued_next_job->version_mask = GLOBAL_STATE->version_mask;

            queue_enqueue(&GLOBAL_STATE->ASIC_jobs_queue, queued_next_job);

            free(coinbase_tx);
            free(merkle_root);
            free(extranonce_2_str);
            extranonce_2++;
        }

        if (GLOBAL_STATE->abandon_work == 1)
        {
            GLOBAL_STATE->abandon_work = 0;
            ASIC_jobs_queue_clear(&GLOBAL_STATE->ASIC_jobs_queue);
        }

        STRATUM_V1_free_mining_notify(mining_notification);
    } if(test_mode) {
        /**
         * Do testing instead of work from stratum
        */

       mining_notify mock_job={
        .job_id: "658dd1e40000ab91",
        .prevhash: "b064adaf22d7ac0ccc834a7fdcf88e55333a985b00010eda0000000000000000",
        .coinb1: "01000000010000000000000000000000000000000000000000000000000000000000000000ffffffff2e03ab980c00046b94a165044953f30d0c",
        .coinb2: "0a636b706f6f6c0a2f736f6c6f206361742fffffffff027b441f2700000000160014f717de441821ef890b25141ad41e25867f02bc040000000000000000266a24aa21a9ed6b7aba5e30acf45bf9b8dbd65ce56a1d33bca0ab92f496f77ce9f8b4032cf8cd00000000",
        .merkle_branches: [
            "1eb4b7263d4cff7da31b12d229f5fff449a2b562b0125ad2d5fafd61553bb304",
            "cdf92795c97d3e605fa106b9fa0bb9117396285a17a5a712bfd185394c5e90d1",
            "df24f3058e2b27517e509959480dccd4d01109f302c82457125af158e02588c5",
            "1e325663cf83f80fb8e96a9003b7c6df511ecfae833e80fa4a7ce9821c9043ec",
            "86b131ccc80a41c7b6bb394d0f50172f39e821fd637f8614a3d38287d7dc7e42",
            "02224547803fc9afe7c1e9a54015a0c4165841e34117f08836f6dc7381757611",
            "637669c49a27971f7bb0b8ea9d06135070077c76878a1e4530d982fcc573ba10",
            "7af9a915bd396db4de93991ead88589be7704892d7e581a3b384f7bf7c418e0a",
            "f8e5d51530b415c31bc481b34035edaa820631e22507af682708231a5ea3be77",
            "35607c594892da344cbc0d0714ef3ab6fbdefc2b7c134bf3e73b0ca4563e48bd",
            "b4f6e62611b0be3d0147e39dfdbeb4c872c5b6a27a265b6eddc013573eee84a1",
           "024e88f023fd4f3b1de233e11afe5010af5fc0812dbceffc9cc76d7e65b20735"
        ],
        .version: "20000000",
        .nbits: "1703d869",
        .ntime: "65a1946b",
        .extranonce1: "63d38d65",
        .extranonce2_size: 8,
        .extranonce2: "000000000852f6ca",
        .merkle_root: "fbf0774adc895ea5e33b033fcadbf4f6f22ffd86751f4b12644502f78e8c8edf"
        }
        // Simulate processing as in the main loop
    char *extranonce_2_str = extranonce_2_generate(mock_job.extranonce2, GLOBAL_STATE->extranonce_2_len);
    char *coinbase_tx = construct_coinbase_tx(mock_job.coinb1, mock_job.coinb2, GLOBAL_STATE->extranonce_str, extranonce_2_str);
    char *merkle_root = calculate_merkle_root_hash(coinbase_tx, (uint8_t(*)[32])mock_job.merkle_branches, mock_job.n_merkle_branches);
    bm_job next_job = construct_bm_job(&mock_job, merkle_root, GLOBAL_STATE->version_mask);

    bm_job *queued_next_job = malloc(sizeof(bm_job));
    memcpy(queued_next_job, &next_job, sizeof(bm_job));
    queued_next_job->extranonce2 = strdup(extranonce_2_str);
    queued_next_job->jobid = strdup(mock_job.job_id);
    queued_next_job->version_mask = GLOBAL_STATE->version_mask;

    queue_enqueue(&GLOBAL_STATE->ASIC_jobs_queue, queued_next_job);

    // Clean up
    free(coinbase_tx);
    free(merkle_root);
    free(extranonce_2_str);
    }
    }
}
