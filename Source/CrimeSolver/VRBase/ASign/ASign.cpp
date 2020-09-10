// This software is under partial ownership by The Ohio State University, for it
// is a product of student employees. For official policy, see
// https://tco.osu.edu/sites/default/files/pdfs/IP-Policy.pdf or contact The
// Ohio State University Office of Legal Affairs.

#include "ASign.h"
#include "ABasePawn/ABasePawn.h"

#define DEFAULT_WIDTH  200
#define DEFAULT_HEIGHT 100

FVector2D ConvertTo2D(FVector v) {
    int x = static_cast<int>(v.Y) ? 1 : 0;
    int y = static_cast<int>(v.Z) ? 1 : 0;
    return FVector2D(x, y);
}

ASign::ASign() {

    //-------------------------------------------------------------------------------------
    // UProperties
    //-------------------------------------------------------------------------------------

    m_string    = FString("hello world");
    m_fFontSize = 1.0f;

    m_sColor            = FColor::White;
    m_bMeshHiddenInGame = true;

    //-------------------------------------------------------------------------------------
    // Procedural mesh
    //-------------------------------------------------------------------------------------

    FVector cornerTL = FVector(0, DEFAULT_WIDTH / -2, DEFAULT_HEIGHT / -2);
    FVector cornerTR = FVector(0, DEFAULT_WIDTH / 2, DEFAULT_HEIGHT / -2);
    FVector cornerBR = FVector(0, DEFAULT_WIDTH / 2, DEFAULT_HEIGHT / 2);
    FVector cornerBL = FVector(0, DEFAULT_WIDTH / -2, DEFAULT_HEIGHT / 2);

    TArray<FVector> vertices;
    vertices.Add(cornerTL); // 0
    vertices.Add(cornerTR); // 1
    vertices.Add(cornerBR); // 2
    vertices.Add(cornerBL); // 3

    TArray<int32> Triangles;
    // front
    Triangles.Add(0);
    Triangles.Add(1);
    Triangles.Add(2);

    Triangles.Add(2);
    Triangles.Add(3);
    Triangles.Add(0);

    // back
    Triangles.Add(2);
    Triangles.Add(1);
    Triangles.Add(0);

    Triangles.Add(0);
    Triangles.Add(3);
    Triangles.Add(2);

    TArray<FVector2D> UV0;
    UV0.Add(ConvertTo2D(cornerTL));
    UV0.Add(ConvertTo2D(cornerTR));
    UV0.Add(ConvertTo2D(cornerBR));
    UV0.Add(ConvertTo2D(cornerBL));

    TArray<FVector>          normals;
    TArray<FProcMeshTangent> tangents;
    TArray<FLinearColor>     vertexColors;

    m_pMesh = CreateDefaultSubobject<UProceduralMeshComponent>("mesh");
    m_pMesh->bUseAsyncCooking       = true;
    m_pMesh->SetGenerateOverlapEvents(false);
    m_pMesh->bHiddenInGame          = m_bMeshHiddenInGame;

    m_pMesh->CreateMeshSection_LinearColor(0, vertices, Triangles, normals, UV0,
                                           vertexColors, tangents, true);

    RootComponent = m_pMesh;

    //-------------------------------------------------------------------------------------
    // Text render
    //-------------------------------------------------------------------------------------

    m_pTextRender = CreateDefaultSubobject<UTextRenderComponent>("text render");
    m_pTextRender->SetupAttachment(RootComponent);

    m_pTextRender->SetWorldRotation(FRotator(0, 180, 0));

    // reload text
    TextReload();
}

void ASign::TextReload() {
    float x = DEFAULT_WIDTH / -2;
    float y = DEFAULT_HEIGHT / 2;

    m_pTextRender->SetHorizontalAlignment(m_eAlignX);

    if (m_eAlignX == EHorizTextAligment::EHTA_Center) {
        x = 0;
    } else if (m_eAlignX == EHorizTextAligment::EHTA_Right) {
        x = DEFAULT_WIDTH / 2;
    }

    m_pTextRender->SetVerticalAlignment(m_eAlignY);
    if (m_eAlignY == EVerticalTextAligment::EVRTA_TextCenter) {
        y = 0;
    } else if (m_eAlignY == EVerticalTextAligment::EVRTA_TextBottom) {
        y = DEFAULT_HEIGHT / -2;
    }

    m_pTextRender->SetRelativeLocation(FVector(-0.1f, x, y));
    m_pTextRender->AddRelativeLocation(
        FVector(0, 0, 2)); // necessary in order to prevent actor rotation from
                           // hiding the text in the mesh

    m_pTextRender->SetTextRenderColor(m_sColor);

    // setup text
    TextWrap(m_string);
}

void ASign::PreInit() {
    // reload text
    TextReload();
}

void ASign::DefaultThink() {
    if (m_bAlwaysFacePlayer) {
        FVector vLocCurrent = GetActorLocation();
        FVector vLocPlayer  = g_pBasePawn->m_pCamera->GetComponentLocation();

        FVector  forward = vLocCurrent - vLocPlayer;
        FRotator rot     = forward.ToOrientationRotator();

        SetActorRotation(rot);
    }
}

void ASign::TextWrap(FString input) {
    if (input.Len() == 0) return;

    const float BASE_FONT_SIZE = 10.8;

    const char* lineBreak = "<br>";
    int         maxLen    = input.Len() * strlen(lineBreak);
    rsize_t     size      = maxLen;

    wchar_t* initialWideText = &input[0];

    size_t i;
    char*  initialText = new char[maxLen];
    size_t wTextSize   = input.Len() + 1; // null terminator

    wcstombs_s(&i, initialText, wTextSize, initialWideText, wTextSize);

    char* wrappedText = new char[maxLen];
    memset(wrappedText, 0, maxLen);
    char* lineText = new char[maxLen];
    memset(lineText, 0, maxLen);

    const char* delimiters = " <>";
    char*       word;
    char*       state;

    word = strtok_s(initialText, delimiters, &state);

    while (word != NULL) { // for each word
        float fMeshWidth = DEFAULT_WIDTH * GetActorScale3D().Y;
        float fLineWidth =
            (strlen(lineText) + strlen(word)) * BASE_FONT_SIZE * m_fFontSize;

        // if the width of the line is bigger than the mesh width
        if (fLineWidth > fMeshWidth || strcmp(word, "br") == 0) {
            strcat_s(wrappedText, size, lineText);

            if (strlen(wrappedText) > 0) { strcat_s(wrappedText, size, lineBreak); }

            memset(lineText, 0, maxLen);
        }

        // if the world is not already <br>, add it to the line text
        if (strcmp(word, "br") != 0) {
            strcat_s(lineText, size, word);
            strcat_s(lineText, size, " ");
        }
        word = strtok_s(NULL, delimiters, &state);
    }

    strcat_s(wrappedText, size, lineText);

    if (m_pTextRender != nullptr) {
        m_pTextRender->SetWorldScale3D(FVector(1, 1, 1));
        m_pTextRender->SetXScale(m_fFontSize);
        m_pTextRender->SetYScale(m_fFontSize);
        m_pTextRender->SetText(FText::FromString(wrappedText));
    }

    // restore
    delete[] lineText;
    delete[] wrappedText;
}