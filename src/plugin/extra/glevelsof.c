ST_retcode sf_levelsof (struct StataInfo *st_info, int level);

ST_retcode sf_levelsof (struct StataInfo *st_info, int level)
{

    /*********************************************************************
     *                           Step 1: Setup                           *
     *********************************************************************/

    ST_retcode rc = 0;
    ST_double z;
    GT_size j, k;
    GT_size sel;
    GT_size numwidth = st_info->numfmt_max > 18? st_info->numfmt_max + 4: 22;
    GT_size kvars = st_info->kvars_by;
    clock_t timer = clock();

    /*********************************************************************
     *                     Copy output back to stata                     *
     *********************************************************************/

    // 18446744073709551615 is the largest unsigned integer, but I think
    // the largest doubles require 22 digits (decimal dot and scientific
    // notation).  So we allocate 22 per number (-levelsof- prints 16
    // significant digits otherwise, so 22 should be enough).

    char *macrobuffer;
    GT_size bufferlen;

    char *sprintfmt     = st_info->cleanstr? strdup("%s"): strdup("`\"%s\"'");
    GT_size sprintextra = st_info->cleanstr? 0: 4;
    GT_size totalseplen = (st_info->J - 1) * st_info->sep_len +
                          st_info->J * st_info->colsep_len * (kvars - 1);

    if ( st_info->kvars_by_str > 0 ) {
        bufferlen   = sizeof(char) * totalseplen + 1;
        bufferlen  += sizeof(char) * st_info->J * (sprintextra * st_info->kvars_by_str);
        bufferlen  += sizeof(char) * st_info->J * (st_info->kvars_by_num * numwidth);
        bufferlen  += sizeof(char) * ((kvars > 1)? 4 * st_info->J: 0);
        bufferlen  += st_info->strbuffer;
        macrobuffer = malloc(bufferlen);
    }
    else {
        bufferlen   = totalseplen + 1;
        bufferlen  += st_info->J * (st_info->kvars_by_num * numwidth);
        bufferlen  += ((kvars > 1)? 4 * st_info->J: 0);
        macrobuffer = malloc(bufferlen * sizeof(char));
    }

    if ( macrobuffer == NULL ) return (sf_oom_error("sf_levelsof", "macrobuffer"));
    memset (macrobuffer, '\0', bufferlen * sizeof(char));

    char *strpos = macrobuffer;
    GT_size rowbytes = (st_info->rowbytes + sizeof(GT_size));

    char *colsep = malloc((st_info->colsep_len + 1) * sizeof(char));
    char *sep    = malloc((st_info->sep_len    + 1) * sizeof(char));
    char *numfmt = malloc((st_info->numfmt_max + 1) * sizeof(char));

    if ( colsep == NULL ) return (sf_oom_error("sf_levelsof", "colsep"));
    if ( sep    == NULL ) return (sf_oom_error("sf_levelsof", "sep"));
    if ( numfmt == NULL ) return (sf_oom_error("sf_levelsof", "numfmt"));

    memset (colsep, '\0', (st_info->colsep_len + 1) * sizeof(char));
    memset (sep,    '\0', (st_info->sep_len    + 1) * sizeof(char));
    memset (numfmt, '\0', (st_info->numfmt_max + 1) * sizeof(char));

    if ( (rc = SF_macro_use("_colsep", colsep, (st_info->colsep_len + 1) * sizeof(char))) ) goto exit;
    if ( (rc = SF_macro_use("_sep",    sep,    (st_info->sep_len    + 1) * sizeof(char))) ) goto exit;
    if ( (rc = SF_macro_use("_numfmt", numfmt, (st_info->numfmt_max + 1) * sizeof(char))) ) goto exit;

    if ( kvars > 1 ) {
        if ( st_info->kvars_by_str > 0 ) {
            for (j = 0; j < st_info->J; j++) {
                if ( j > 0 ) strpos += sprintf(strpos, "%s", sep);
                strpos += sprintf(strpos, "`\"");
                for (k = 0; k < kvars; k++) {
                    if ( k > 0 ) strpos += sprintf(strpos, "%s", colsep);
                    sel = j * rowbytes + st_info->positions[k];
                    if ( st_info->byvars_lens[k] > 0 ) {
                        strpos += sprintf(strpos, sprintfmt, st_info->st_by_charx + sel);
                    }
                    else {
                        z = *((ST_double *) (st_info->st_by_charx + sel));
                        if ( SF_is_missing(z) ) {
                            // strpos += sprintf(strpos, ".");
                            GTOOLS_SWITCH_MISSING
                        }
                        else {
                            strpos += sprintf(strpos, numfmt, z);
                        }
                    }
                }
                strpos += sprintf(strpos, "\"'");
            }
        }
        else {
            for (j = 0; j < st_info->J; j++) {
                if ( j > 0 ) strpos += sprintf(strpos, "%s", sep);
                strpos += sprintf(strpos, "`\"");
                for (k = 0; k < kvars; k++) {
                    if ( k > 0 ) strpos += sprintf(strpos, "%s", colsep);
                    sel = j * (kvars + 1) + k;
                    z  = st_info->st_by_numx[sel];
                    if ( SF_is_missing(z) ) {
                        // strpos += sprintf(strpos, ".");
                        GTOOLS_SWITCH_MISSING
                    }
                    else {
                        strpos += sprintf(strpos, numfmt, z);
                    }
                }
                strpos += sprintf(strpos, "\"'");
            }
        }
    }
    else {
        if ( st_info->kvars_by_str > 0 ) {
            for (j = 0; j < st_info->J; j++) {
                if ( j > 0 ) strpos += sprintf(strpos, "%s", sep);
                sel = j * rowbytes;
                if ( st_info->byvars_lens[0] > 0 ) {
                    strpos += sprintf(strpos, sprintfmt, st_info->st_by_charx + sel);
                }
                else {
                    z = *((ST_double *) (st_info->st_by_charx + sel));
                    if ( SF_is_missing(z) ) {
                        // strpos += sprintf(strpos, ".");
                        GTOOLS_SWITCH_MISSING
                    }
                    else {
                        strpos += sprintf(strpos, numfmt, z);
                    }
                    // printf("%.16G\n", z);
                }
            }
        }
        else {
            for (j = 0; j < st_info->J; j++) {
                if ( j > 0 ) strpos += sprintf(strpos, "%s", sep);
                sel = j * (kvars + 1);
                z = st_info->st_by_numx[sel];
                if ( SF_is_missing(z) ) {
                    // strpos += sprintf(strpos, ".");
                    GTOOLS_SWITCH_MISSING
                }
                else {
                    strpos += sprintf(strpos, numfmt, z);
                }
                // printf("%.16G\n", z);
            }
        }
    }

    if ( (rc = SF_macro_save("_vals", macrobuffer)) ) goto exit;
    if ( st_info->benchmark > 1 )
        sf_running_timer (&timer, "\tPlugin step 5: Wrote levels to Stata macro");

exit:
    free (sep);
    free (colsep);
    free (numfmt);
    free (sprintfmt);
    free (macrobuffer);

    return (rc);
}
