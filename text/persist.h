#ifndef PERSIST_H
#define PERSIST_H

static const wchar_t stream_name[] = L".font";

int SETTINGS_saveLogFont(LOGFONT lf) {
    TCHAR file_name[MAX_PATH + sizeof(stream_name)];
    int count = GetModuleFileName(NULL, file_name, MAX_PATH);
    if (count == 0 || GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        printf("Error saving the config stream! %ld\n",GetLastError());
        return 0;
    }
    memcpy(file_name + count, stream_name, sizeof(stream_name)); //-1 ebcasue its null
    
    FILE* fp;
    fp = _wfopen(file_name, L"wb");
    if (!fp){
        printf( "Cannot open config file for writing\n" );
        return 0;
    }
    
    /*
    //lfHeight, lfWidth, lfEscapement, lfOrientation, lfWeight
    fwrite(&lf.lfHeight, sizeof(lf.lfHeight), 1, fp);
    fwrite(&lf.lfWidth, sizeof(lf.lfHeight), 1, fp);
    fwrite(&lf.lfEscapement, sizeof(lf.lfHeight), 1, fp);
    fwrite(&lf.lfOrientation, sizeof(lf.lfHeight), 1, fp);
    fwrite(&lf.lfWeight, sizeof(lf.lfHeight), 1, fp);
    //lfItalic, lfUnderline, lfStrikeOut, lfCharSet, lfOutPrecision, lfClipPrecision, lfQuality, lfPitchAndFamily
    fwrite(&lf.lfItalic, sizeof(lf.lfItalic), 1, fp);
    fwrite(&lf.lfUnderline, sizeof(lf.lfItalic), 1, fp);
    fwrite(&lf.lfStrikeOut, sizeof(lf.lfItalic), 1, fp);
    fwrite(&lf.lfCharSet, sizeof(lf.lfItalic), 1, fp);
    fwrite(&lf.lfOutPrecision, sizeof(lf.lfItalic), 1, fp);
    fwrite(&lf.lfClipPrecision, sizeof(lf.lfItalic), 1, fp);
    fwrite(&lf.lfQuality, sizeof(lf.lfItalic), 1, fp);
    fwrite(&lf.lfPitchAndFamily, sizeof(lf.lfItalic), 1, fp);
    //lfFaceName
    fwrite(lf.lfFaceName, sizeof(CHAR), LF_FACESIZE, fp);*/
    
    fwrite(&lf, sizeof(lf), 1, fp);
    
    fclose(fp);
    return 1;
}

int SETTINGS_loadLogFont(LOGFONT* lf) {
    TCHAR file_name[MAX_PATH + sizeof(stream_name)];
    int count = GetModuleFileName(NULL, file_name, MAX_PATH);
    if (count == 0 || GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        printf("Error loading the config stream! %ld\n",GetLastError());
        return 0;
    }
    memcpy(file_name + count, stream_name, sizeof(stream_name)); //-1 ebcasue its null
    
    FILE* fp;
    fp = _wfopen(file_name, L"rb");
    
    if (!fp) {
        //printf( "Cannot open config file for loading\n" );
        return 0;
    }
    
    fread(lf, sizeof(*lf), 1, fp);
    
    fclose(fp);
    
    return 1;
}

#endif