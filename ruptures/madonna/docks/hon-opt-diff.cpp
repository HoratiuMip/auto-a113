#include <bridge.hpp>
using namespace mdn;

#define MODULE_NAME "hon-opt-diff"

using namespace glm;

struct Dock : dock_t {
public:
    Dock( void ) = default;

public:
    inline static constexpr int METHOD_COUNT = 5;
    inline static const char* const METHODS[ METHOD_COUNT ] = {
        "Newton",
        "Steepest",
        "Conjugate-fr",
        "Conjugate-pr",
        "Conjugate-hs"
    };
    enum Method_ {
        Method_Newton,
        Method_Steepest,
        Method_ConjugateFr,
        Method_ConjugatePr,
        Method_ConjugateHs
    };

public:
    struct _ex_t {
        _ex_t( void ) = default;

        void bind( std::string_view in_exp_ ) {
            exp.parse( in_exp_ );

            f = [ this ] ( double x1, double x2 ) {
                exp.bind( [ x1, x2 ] ( std::string_view var_, double* val_ ) -> status_t {
                    if( var_ == "x1" ) { *val_ = x1; return A113_OK; }
                    if( var_ == "x2" ) { *val_ = x2; return A113_OK; }
                    return A113_ERR_NOT_FOUND;
                } );
                double res = 0.0; exp.resolve( &res );
                return res;
            };

            constexpr double step = 0.1;
            grid.span_s( { { step, -8, 8 }, { step, 8, -8 } } );
            grid.apply( [ this ] ( double* x ) -> double { return f( x[0], x[1] ); } );
        }

        text::Fastexp< double >   exp;
        mdn_0::fnc_2d_t<>         f;
        mdn_2::srf_grid_t<>       grid;
    };

public:
    Dispenser< _ex_t >   ex   = { DispenserMode_Drop };    

    std::string            in_exp;

    int                    step_count[ METHOD_COUNT ] = { 0 };
    mdn_0::arr_t<2>        grad;
    mdn_0::arr_t<4>        hess;
    mdn_0::arr_t<2>        x0{ 0.5, 0.5 };
    double                 s{ 1.0 };

public:
    void compute_gradient( mdn_0::arr_t<2> X ) { grad = mdn_0::d1_f2( X[0], X[1], 1e-6, ex.hold()->f ); }

    void compute_hessian( mdn_0::arr_t<2> X ) { hess = mdn_0::d2h_f2( X[0], X[1], 1e-6, ex.hold()->f ); }

public:
    MDN_DOCK_NAME_FNC override { return MODULE_NAME; }

    MDN_DOCK_GUIX_FNC override {
        auto ex = this->ex.watch();

        bool open = true;
        if( ImGui::Begin( "Optimization with differentiation", &open, ImGuiWindowFlags_None ) ) {
            ImGui::Separator();
            ImGui::TextUnformatted( "f(x1,x2) =" ); ImGui::SameLine();
            const bool new_exp = ImGui::InputText( "##in-exp", &in_exp, ImGuiInputTextFlags_EnterReturnsTrue );
            ImGui::Separator();

            if( ex ) {
                ImPlot::PushColormap( ImPlotColormap_Viridis );
                if( ImPlot::BeginPlot( "##plt-f", {680, 680} ) ) {
                    if( ImPlot::IsPlotHovered() && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) && ImGui::IsKeyDown( ImGuiKey_LeftCtrl ) ) {
                        auto p = ImPlot::GetPlotMousePos();
                        x0 = { p.x, p.y };
                    }

                    ImPlot::PlotHeatmap(
                        "##htm-f", ex->grid.raw(), ex->grid.n_of(1), ex->grid.n_of(0),
                        ex->grid.min(), ex->grid.max(), nullptr, {-8,-8}, {8,8},
                        ImPlotHeatmapFlags_None
                    );

                    /* === Newton === */ {
                    auto xk = x0;
                    for( int n = 1; n <= step_count[ Method_Newton ]; ++n ) {
                        compute_gradient( xk ); 
                        compute_hessian( xk );
                        MDN_ASSERT_OR( A113_OK == mdn_0::invm( hess.get(), 2 ) ) break;

                        mdn_0::arr_t<2> xk1 = {
                            xk.x() - hess[0]*grad.x() - hess[1]*grad.y(),
                            xk.y() - hess[2]*grad.x() - hess[3]*grad.y()
                        };

                        ImPlot::SetNextLineStyle( { 1,.36,0,1 }, 2 );
                        ImPlot::PlotLine( METHODS[ Method_Newton ], (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        if( ImPlot::IsLegendEntryHovered( METHODS[ Method_Newton ] ) ) {
                            ImPlot::SetNextLineStyle( { 1,1,1,1 }, 2 );
                            ImPlot::PlotScatter( "", (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        }

                        xk = xk1;
                    }
                    }

                    /* === Steepest === */ {
                    auto xk = x0;
                    for( int n = 1; n <= step_count[ Method_Steepest ]; ++n ) {
                        compute_gradient( xk );
                        auto dk = -grad; dk /= dk.norm();
                        
                        auto [ s, _ ] = mdn_0::search_mf1_elimgr< double >( 0.0, 1.0, 0.00001, nullptr, [ & ] ( double s_ ) -> double {
                            auto dxk = xk + dk*s_; return ex->f( dxk.x(), dxk.y() );
                        } );

                        auto xk1 = xk + dk*s;

                        ImPlot::SetNextLineStyle( { 1,0,.36,1 }, 2 );
                        ImPlot::PlotLine( METHODS[ Method_Steepest ], (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        if( ImPlot::IsLegendEntryHovered( METHODS[ Method_Steepest ] ) ) {
                            ImPlot::SetNextLineStyle( { 1,1,1,1 }, 2 );
                            ImPlot::PlotScatter( "", (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        }

                        xk = xk1;
                    }
                    }

                    /* === Conjugate FR === */ {
                    auto             xk = x0;
                    mdn_0::arr_t<2>  dk = { 0, 0 };
                    double           bk = 0;
                    compute_gradient( xk );
                    for( int n = 1; n <= step_count[ Method_ConjugateFr ]; ++n ) {
                        auto gk = grad;
                        
                        dk = -gk + dk*bk;

                        auto [ s, _ ] = mdn_0::search_mf1_elimgr< double >( 0.0, 1.0, 0.00001, nullptr, [ & ] ( double s_ ) -> double {
                            auto dxk = xk + dk*s_; return ex->f( dxk.x(), dxk.y() );
                        } );

                        auto xk1 = xk + dk*s;

                        ImPlot::SetNextLineStyle( { 1,.72,0,1 }, 2 );
                        ImPlot::PlotLine( METHODS[ Method_ConjugateFr ], (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        if( ImPlot::IsLegendEntryHovered( METHODS[ Method_ConjugateFr ] ) ) {
                            ImPlot::SetNextLineStyle( { 1,1,1,1 }, 2 );
                            ImPlot::PlotScatter( "", (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        }

                        compute_gradient( xk1 );
                        auto gk1 = grad;
                        
                        bk = pow(gk1.norm(),2) / pow(gk.norm(),2);

                        xk = xk1;
                    }
                    }

                    /* === Conjugate PR === */ {
                    auto             xk = x0;
                    mdn_0::arr_t<2>  dk = { 0, 0 };
                    double           bk = 0;
                    compute_gradient( xk );
                    for( int n = 1; n <= step_count[ Method_ConjugatePr ]; ++n ) {
                        auto gk = grad;
                        
                        dk = -gk + dk*bk;

                        auto [ s, _ ] = mdn_0::search_mf1_elimgr< double >( 0.0, 1.0, 0.00001, nullptr, [ & ] ( double s_ ) -> double {
                            auto dxk = xk + dk*s_; return ex->f( dxk.x(), dxk.y() );
                        } );

                        auto xk1 = xk + dk*s;

                        ImPlot::SetNextLineStyle( { 1,0,.72,1 }, 2 );
                        ImPlot::PlotLine( METHODS[ Method_ConjugatePr ], (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        if( ImPlot::IsLegendEntryHovered( METHODS[ Method_ConjugatePr ] ) ) {
                            ImPlot::SetNextLineStyle( { 1,1,1,1 }, 2 );
                            ImPlot::PlotScatter( "", (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        }

                        compute_gradient( xk1 );
                        auto gk1 = grad;

                        bk = gk1.dot( gk1-gk ) / gk.dot( gk );

                        xk = xk1;
                    }
                    }

                    /* === Conjugate HS === */ {
                    auto             xk = x0;
                    mdn_0::arr_t<2>  dk = { 0, 0 };
                    double           bk = 0;
                    compute_gradient( xk );
                    for( int n = 1; n <= step_count[ Method_ConjugateHs ]; ++n ) {
                        auto gk = grad;
                        
                        dk = -gk + dk*bk;

                        auto [ s, _ ] = mdn_0::search_mf1_elimgr< double >( 0.0, 1.0, 0.00001, nullptr, [ & ] ( double s_ ) -> double {
                            auto dxk = xk + dk*s_; return ex->f( dxk.x(), dxk.y() );
                        } );

                        auto xk1 = xk + dk*s;

                        ImPlot::SetNextLineStyle( { 1,.36,.36,1 }, 2 );
                        ImPlot::PlotLine( METHODS[ Method_ConjugateHs ], (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        if( ImPlot::IsLegendEntryHovered( METHODS[ Method_ConjugateHs ] ) ) {
                            ImPlot::SetNextLineStyle( { 1,1,1,1 }, 2 );
                            ImPlot::PlotScatter( "", (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        }

                        compute_gradient( xk1 );
                        auto gk1 = grad;

                        bk = gk1.dot( gk1-gk ) / dk.dot( gk1-gk );

                        xk = xk1;
                    }
                    }

                    ImPlot::EndPlot();
                }

                ImGui::SameLine();
                ImPlot::ColormapScale( "z", ex->grid.min(), ex->grid.max(), {100,680} );
                ImPlot::PopColormap();
                
                ImGui::SameLine();
                ImGui::BeginTable( "Steps", METHOD_COUNT, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_HighlightHoveredColumn );
                for( int i = 0x0; i < METHOD_COUNT; ++i ) {
                    ImGui::TableSetupColumn( METHODS[i], ImGuiTableColumnFlags_AngledHeader );
                }
                ImGui::TableAngledHeadersRow();
                ImGui::TableNextRow();
                
                for( int i = 0x0; i < METHOD_COUNT; ++i ) {
                    ImGui::TableNextColumn();
                    ImGui::VSliderInt( std::format( "##step-count-{}", METHODS[i] ).c_str(), {50, 500}, step_count + i, 0, 100 );
                }
                ImGui::EndTable();

                ImGui::SeparatorText( "Initial guess");
                ImGui::InputScalarN( "##input-ig", ImGuiDataType_Double, &x0, 2 );
            }

            if( new_exp ) {
                std::thread( [ this ] { if( not in_exp.empty() ) this->ex.control()->bind( in_exp ); } ).detach();
            }
        } 
        ImGui::End();
        return open ? A113_OK : A113_ERR_TERMINATED; 
    }
};

struct Proxy : proxy_t {
public:
    Proxy( void )
    : proxy_t{
        MODULE_NAME,
        {},
        { 
        MDN_PROXY_CLI_BASIC_INSTALL
        }
    } {}
};

MDN_AUTO_INSTALL( Proxy );
